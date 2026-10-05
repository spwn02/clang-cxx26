//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___EXECUTION_ASYNC_SCOPE_H
#define _LIBCPP___EXECUTION_ASYNC_SCOPE_H

#include <__concepts/constructible.h>
#include <__concepts/copyable.h>
#include <__concepts/movable.h>
#include <__concepts/same_as.h>
#include <__config>
#include <__execution/completion_functions.h>
#include <__execution/completion_signatures.h>
#include <__execution/connect.h>
#include <__execution/env.h>
#include <__execution/fwd_env.h>
#include <__execution/get_allocator.h>
#include <__execution/get_completion_signatures.h>
#include <__execution/get_env.h>
#include <__execution/get_scheduler.h>
#include <__execution/get_stop_token.h>
#include <__execution/just.h>
#include <__execution/operation_state.h>
#include <__execution/receiver.h>
#include <__execution/sender.h>
#include <__execution/sender_adaptor_closure.h>
#include <__execution/stop_when.h>
#include <__functional/bind_back.h>
#include <__memory/allocator.h>
#include <__memory/allocator_traits.h>
#include <__memory/unique_ptr.h>
#include <__mutex/lock_guard.h>
#include <__mutex/mutex.h>
#include <__stop_token/inplace_stop_source.h>
#include <__stop_token/inplace_stop_token.h>
#include <__stop_token/stoppable_token.h>
#include <__type_traits/conditional.h>
#include <__type_traits/is_nothrow_constructible.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/exchange.h>
#include <__utility/forward.h>
#include <__utility/move.h>
#include <cstddef>
#include <exception>
#include <limits>
#include <optional>
#include <tuple>
#include <variant>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_PUSH_MACROS
#include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_THREADS

namespace execution {

template <class _Assoc>
concept scope_association =
    movable<_Assoc> && is_nothrow_move_constructible_v<_Assoc> && is_nothrow_move_assignable_v<_Assoc> &&
    default_initializable<_Assoc> && requires(const _Assoc __assoc) {
      { static_cast<bool>(__assoc) } noexcept;
      { __assoc.try_associate() } -> same_as<_Assoc>;
    };

template <class _Token>
concept scope_token = copyable<_Token> && requires(const _Token __token) {
  { __token.try_associate() } -> scope_association;
  { __token.wrap(execution::just()) } -> sender_in<env<>>;
};

template <class _Scope>
class __scope_association {
public:
  _LIBCPP_HIDE_FROM_ABI __scope_association() noexcept = default;
  _LIBCPP_HIDE_FROM_ABI explicit __scope_association(_Scope* __scope) noexcept : __scope_(__scope) {}
  _LIBCPP_HIDE_FROM_ABI __scope_association(__scope_association&& __other) noexcept
      : __scope_(std::exchange(__other.__scope_, nullptr)) {}
  _LIBCPP_HIDE_FROM_ABI __scope_association& operator=(__scope_association&& __other) noexcept {
    if (this != &__other) {
      auto* __old = std::exchange(__scope_, std::exchange(__other.__scope_, nullptr));
      if (__old)
        __old->__disassociate();
    }
    return *this;
  }
  _LIBCPP_HIDE_FROM_ABI ~__scope_association() {
    if (__scope_)
      __scope_->__disassociate();
  }
  _LIBCPP_HIDE_FROM_ABI explicit operator bool() const noexcept { return __scope_ != nullptr; }
  _LIBCPP_HIDE_FROM_ABI __scope_association try_associate() const noexcept {
    return __scope_ && __scope_->__try_associate() ? __scope_association(__scope_) : __scope_association();
  }

private:
  _Scope* __scope_ = nullptr;
};

class simple_counting_scope {
public:
  class token {
  public:
    template <sender _Sndr>
    _LIBCPP_HIDE_FROM_ABI _Sndr&& wrap(_Sndr&& __sndr) const noexcept {
      return std::forward<_Sndr>(__sndr);
    }
    _LIBCPP_HIDE_FROM_ABI auto try_associate() const noexcept {
      return __scope_->__try_associate()
               ? __scope_association<simple_counting_scope>(__scope_)
               : __scope_association<simple_counting_scope>();
    }

  private:
    friend class simple_counting_scope;
    _LIBCPP_HIDE_FROM_ABI explicit token(simple_counting_scope* __scope) noexcept : __scope_(__scope) {}
    simple_counting_scope* __scope_;
  };

  _LIBCPP_HIDE_FROM_ABI simple_counting_scope() noexcept = default;

  simple_counting_scope(simple_counting_scope&&)            = delete;
  simple_counting_scope& operator=(simple_counting_scope&&) = delete;

  static constexpr size_t max_associations = (numeric_limits<size_t>::max)();

  _LIBCPP_HIDE_FROM_ABI ~simple_counting_scope() {
    if (__state_ != __state::__idle && __state_ != __state::__idle_closed && __state_ != __state::__joined)
      std::terminate();
  }

  _LIBCPP_HIDE_FROM_ABI token get_token() noexcept { return token(this); }

  _LIBCPP_HIDE_FROM_ABI void close() noexcept {
    lock_guard<mutex> __lock(__mtx_);
    switch (__state_) {
    case __state::__idle:
      __state_ = __state::__idle_closed;
      break;
    case __state::__open:
      __state_ = __state::__closed;
      break;
    case __state::__joining:
      __state_ = __state::__closed_joining;
      break;
    default:
      break;
    }
  }

  _LIBCPP_HIDE_FROM_ABI auto join() noexcept;

private:
  friend class counting_scope;
  template <class>
  friend class __scope_association;

  struct __join_sink_base {
    __join_sink_base* __next_                                = nullptr;
    _LIBCPP_HIDE_FROM_ABI virtual void __complete() noexcept = 0;

  protected:
    _LIBCPP_HIDE_FROM_ABI ~__join_sink_base() = default;
  };

  template <class _Rcvr>
  class __join_opstate;
  class __join_sender;

  _LIBCPP_HIDE_FROM_ABI bool __try_associate() noexcept {
    lock_guard<mutex> __lock(__mtx_);
    if (__count_ == max_associations ||
        (__state_ != __state::__idle && __state_ != __state::__open && __state_ != __state::__joining))
      return false;
    if (__state_ == __state::__idle)
      __state_ = __state::__open;
    ++__count_;
    return true;
  }

  _LIBCPP_HIDE_FROM_ABI void __disassociate() noexcept {
    __join_sink_base* __to_complete = nullptr;
    {
      lock_guard<mutex> __lock(__mtx_);
      --__count_;
      if (__count_ == 0 && (__state_ == __state::__joining || __state_ == __state::__closed_joining)) {
        __state_      = __state::__joined;
        __to_complete = std::exchange(__sink_, nullptr);
      }
    }
    while (__to_complete) {
      auto* __next = __to_complete->__next_;
      __to_complete->__complete();
      __to_complete = __next;
    }
  }

  _LIBCPP_HIDE_FROM_ABI bool __join_start(__join_sink_base* __sink) noexcept {
    lock_guard<mutex> __lock(__mtx_);
    if (__count_ == 0) {
      __state_ = __state::__joined;
      return true;
    }
    if (__state_ == __state::__open)
      __state_ = __state::__joining;
    if (__state_ == __state::__closed)
      __state_ = __state::__closed_joining;
    __sink->__next_ = __sink_;
    __sink_         = __sink;
    return false;
  }

  enum class __state { __idle, __open, __joining, __closed, __idle_closed, __closed_joining, __joined };
  mutex __mtx_;
  size_t __count_           = 0;
  __state __state_          = __state::__idle;
  __join_sink_base* __sink_ = nullptr;
};

template <class _Rcvr>
class simple_counting_scope::__join_opstate final : public simple_counting_scope::__join_sink_base {
  static auto __scheduler(const _Rcvr& __rcvr) { return execution::get_start_scheduler(execution::get_env(__rcvr)); }
  struct __receiver {
    using receiver_concept = receiver_tag;
    _Rcvr& __rcvr_;
    void set_value() && noexcept { execution::set_value(std::move(__rcvr_)); }
    template <class _Error>
    void set_error(_Error&& __error) && noexcept {
      execution::set_error(std::move(__rcvr_), std::forward<_Error>(__error));
    }
    void set_stopped() && noexcept { execution::set_stopped(std::move(__rcvr_)); }
    decltype(auto) get_env() const noexcept { return execution::get_env(__rcvr_); }
  };
  using __scheduled_t = decltype(execution::schedule(__scheduler(std::declval<const _Rcvr&>())));

public:
  using operation_state_concept = operation_state_tag;
  _LIBCPP_HIDE_FROM_ABI __join_opstate(simple_counting_scope* __scope, _Rcvr&& __rcvr)
      : __scope_(__scope),
        __rcvr_(std::move(__rcvr)),
        __op_(execution::connect(execution::schedule(__scheduler(__rcvr_)), __receiver{__rcvr_})) {}
  __join_opstate(const __join_opstate&)            = delete;
  __join_opstate& operator=(const __join_opstate&) = delete;
  _LIBCPP_HIDE_FROM_ABI void start() & noexcept {
    if (__scope_->__join_start(this))
      execution::set_value(std::move(__rcvr_));
  }

private:
  _LIBCPP_HIDE_FROM_ABI void __complete() noexcept override { execution::start(__op_); }
  simple_counting_scope* __scope_;
  _Rcvr __rcvr_;
  connect_result_t<__scheduled_t, __receiver> __op_;
};

class simple_counting_scope::__join_sender {
public:
  using sender_concept = sender_tag;

  _LIBCPP_HIDE_FROM_ABI explicit __join_sender(simple_counting_scope* __scope) noexcept : __scope_(__scope) {}

  _LIBCPP_HIDE_FROM_ABI env<> get_env() const noexcept { return {}; }

  template <class _Rcvr>
  _LIBCPP_HIDE_FROM_ABI __join_opstate<remove_cvref_t<_Rcvr>> connect(_Rcvr&& __rcvr) const {
    return __join_opstate<remove_cvref_t<_Rcvr>>(__scope_, std::forward<_Rcvr>(__rcvr));
  }

  template <class _Self, class... _Env>
  _LIBCPP_HIDE_FROM_ABI static consteval auto get_completion_signatures() {
    return completion_signatures<set_value_t()>{};
  }

private:
  simple_counting_scope* __scope_;
};

_LIBCPP_HIDE_FROM_ABI inline auto simple_counting_scope::join() noexcept { return __join_sender(this); }

// [exec.scope.counting] (Pass 2, P3149R11): counting_scope behaves like a
// simple_counting_scope augmented with a stop source. Implemented exactly as the paper's own
// [exec.scope.counting]p2 "as if implemented like so" reference implementation: composition
// over a private simple_counting_scope member plus an inplace_stop_source, not a parallel
// reimplementation of the count/state-machine logic -- get_token()/close()/join() are thin
// delegates, request_stop() is a single call into the stop source, and token::wrap() is the
// only place that actually does something new (fusing the stop source's own token into the
// wrapped sender via the exposition-only stop-when, <__execution/stop_when.h>).
class counting_scope {
public:
  class token {
  public:
    template <sender _Sndr>
    _LIBCPP_HIDE_FROM_ABI auto wrap(_Sndr&& __sndr) const
        noexcept(is_nothrow_constructible_v<remove_cvref_t<_Sndr>, _Sndr>) {
      return execution::__stop_when(std::forward<_Sndr>(__sndr), __scope_->__source_.get_token());
    }
    _LIBCPP_HIDE_FROM_ABI auto try_associate() const noexcept {
      return __scope_->__try_associate()
               ? __scope_association<counting_scope>(__scope_)
               : __scope_association<counting_scope>();
    }

  private:
    friend class counting_scope;
    _LIBCPP_HIDE_FROM_ABI explicit token(counting_scope* __scope) noexcept : __scope_(__scope) {}
    counting_scope* __scope_;
  };

  static constexpr size_t max_associations = simple_counting_scope::max_associations;

  _LIBCPP_HIDE_FROM_ABI counting_scope() noexcept = default;

  counting_scope(counting_scope&&)            = delete;
  counting_scope& operator=(counting_scope&&) = delete;

  // ~simple_counting_scope() already terminate()s unless in a safe-to-destroy state -- nothing
  // extra to check here, since counting_scope's own state lives entirely in __scope_.
  _LIBCPP_HIDE_FROM_ABI ~counting_scope() = default;

  _LIBCPP_HIDE_FROM_ABI token get_token() noexcept { return token(this); }

  _LIBCPP_HIDE_FROM_ABI void close() noexcept { __scope_.close(); }

  _LIBCPP_HIDE_FROM_ABI void request_stop() noexcept { __source_.request_stop(); }

  _LIBCPP_HIDE_FROM_ABI auto join() noexcept { return __scope_.join(); }

private:
  friend class token;
  template <class>
  friend class __scope_association;
  bool __try_associate() noexcept { return __scope_.__try_associate(); }
  void __disassociate() noexcept { __scope_.__disassociate(); }
  simple_counting_scope __scope_;
  inplace_stop_source __source_;
};

// [exec.associate]: the sender owns its association until transferred to an operation.
template <class _Sndr, class _Assoc>
struct __associate_data {
  optional<_Sndr> __sender_;
  _Assoc __assoc_;
  template <class _Token, class _Input>
  __associate_data(_Token __token, _Input&& __in_sndr)
      : __sender_(in_place, __token.wrap(std::forward<_Input>(__in_sndr))), __assoc_(__token.try_associate()) {
    if (!__assoc_)
      __sender_.reset();
  }
  __associate_data(const __associate_data& __other)
    requires copy_constructible<_Sndr>
      : __assoc_(__other.__assoc_.try_associate()) {
    if (__assoc_)
      __sender_.emplace(*__other.__sender_);
  }
  __associate_data(__associate_data&& __other) noexcept(is_nothrow_move_constructible_v<_Sndr>)
      : __assoc_(std::move(__other.__assoc_)) {
    if (__assoc_) {
      __sender_.emplace(std::move(*__other.__sender_));
      __other.__sender_.reset();
    }
  }
  ~__associate_data() { __sender_.reset(); }
};

template <class _Sndr, class _Assoc, class _Rcvr>
class __associate_opstate {
  using __child_op_t = connect_result_t<_Sndr, _Rcvr>;

public:
  using operation_state_concept = operation_state_tag;
  __associate_opstate(__associate_data<_Sndr, _Assoc>&& __data, _Rcvr&& __rcvr)
      : __assoc_(std::move(__data.__assoc_)), __rcvr_(std::move(__rcvr)) {
    if (__assoc_) {
      ::new (static_cast<void*>(std::addressof(__child_)))
          __child_op_t(execution::connect(std::move(*__data.__sender_), std::move(__rcvr_)));
      __data.__sender_.reset();
    }
  }
  __associate_opstate(const __associate_opstate&) = delete;
  ~__associate_opstate() {
    if (__assoc_)
      __child_.~__child_op_t();
  }
  void start() & noexcept {
    if (__assoc_)
      execution::start(__child_);
    else
      execution::set_stopped(std::move(__rcvr_));
  }

private:
  _Assoc __assoc_;
  _Rcvr __rcvr_;
  union {
    __child_op_t __child_;
  };
};

template <class>
struct __associate_sigs;
template <class... _Sigs>
struct __associate_sigs<completion_signatures<_Sigs...>> {
  using type = completion_signatures<_Sigs..., set_stopped_t()>;
};

template <class _Tag, class _Assoc, class _Sndr>
class __associate_sndr {
public:
  using sender_concept = sender_tag;
  _LIBCPP_NO_UNIQUE_ADDRESS _Tag tag;
  __associate_data<_Sndr, _Assoc> data;
  template <class _Rcvr>
  auto connect(_Rcvr&& __rcvr) && {
    return __associate_opstate<_Sndr, _Assoc, remove_cvref_t<_Rcvr>>(std::move(data), std::forward<_Rcvr>(__rcvr));
  }
  template <class _Rcvr>
    requires copy_constructible<_Sndr>
  auto connect(_Rcvr&& __rcvr) const& {
    return __associate_opstate<_Sndr, _Assoc, remove_cvref_t<_Rcvr>>(
        __associate_data<_Sndr, _Assoc>(data), std::forward<_Rcvr>(__rcvr));
  }
  auto get_env() const noexcept { return env<>{}; }
  // [exec.associate] check-types: the wrapped sender's signatures against FWD-ENV-T(Env)...; the completions are those of
  // the child connected to the receiver (environment: the first of Env..., env<>) plus set_stopped_t() (no association).
  template <class _Self, class... _Env>
    requires sender_in<_Sndr, __fwd_env_of_first_t<_Env...>> &&
             sender_in<_Sndr, typename __first_env_or<env<>, remove_cvref_t<_Env>...>::type>
  static consteval auto get_completion_signatures() {
    return typename __associate_sigs<
        completion_signatures_of_t<_Sndr, typename __first_env_or<env<>, remove_cvref_t<_Env>...>::type>>::type{};
  }
};

struct associate_t {
  template <sender _Sndr, scope_token _Token>
  auto operator()(_Sndr&& __sndr, _Token __token) const {
    using __wrapped_t = remove_cvref_t<decltype(__token.wrap(std::forward<_Sndr>(__sndr)))>;
    using __assoc_t   = decltype(__token.try_associate());
    return __associate_sndr<associate_t, __assoc_t, __wrapped_t>{
        {}, __associate_data<__wrapped_t, __assoc_t>(__token, std::forward<_Sndr>(__sndr))};
  }

  // [exec.adapt.obj]: the partial application associate(token) is a pipeable sender adaptor closure.
  template <scope_token _Token>
  auto operator()(_Token __token) const {
    return execution::__pipeable(std::__bind_back(*this, std::move(__token)));
  }
};
inline constexpr associate_t associate{};

// [exec.scope.spawn]. Unlike associate(), spawn is NOT connect()/start() from the caller's
// side at all -- there is no outer receiver, nothing for the caller to hold onto. It owns its
// own dynamically-allocated operation state and destroys itself on completion. Teardown order
// on completion is significant (see docs/design/async_scope_p3149.md): destroy the connected
// child operation state, deallocate its storage, *then* disassociate -- in that order, since
// the allocator used to free the storage must still be valid when it does so.
struct __spawn_op_base {
  _LIBCPP_HIDE_FROM_ABI virtual void __on_complete() noexcept = 0;

protected:
  _LIBCPP_HIDE_FROM_ABI ~__spawn_op_base() = default;
};

// Stores __spawn_op_base* (not the derived _Op*): __on_complete() is a private override on
// the derived __spawn_op (overriding a base member doesn't widen its access), so calling it
// through a derived-typed pointer would need __spawn_inner_rcvr to be a friend of every
// concrete __spawn_op instantiation. Going through the (publicly-inherited) base pointer
// reaches the same override via ordinary virtual dispatch without needing that friendship.
class __spawn_inner_rcvr {
public:
  using receiver_concept = receiver_tag;

  _LIBCPP_HIDE_FROM_ABI explicit __spawn_inner_rcvr(__spawn_op_base* __op) noexcept : __op_(__op) {}

  // No set_error overload: per [exec.scope.spawn]'s Mandates, the input sender must have no
  // error completions -- omitting set_error here means connect()'s own receiver_of check
  // rejects a sender that could complete with an error at compile time, enforcing the
  // Mandates structurally rather than needing a separate check.
  _LIBCPP_HIDE_FROM_ABI void set_value() && noexcept { __op_->__on_complete(); }
  _LIBCPP_HIDE_FROM_ABI void set_stopped() && noexcept { __op_->__on_complete(); }

  _LIBCPP_HIDE_FROM_ABI env<> get_env() const noexcept { return {}; }

private:
  __spawn_op_base* __op_;
};

template <class _Sndr, class _Token, class _Alloc>
class __spawn_op final : public __spawn_op_base {
  using __rcvr_t = __spawn_inner_rcvr;

public:
  _LIBCPP_HIDE_FROM_ABI __spawn_op(_Sndr&& __sndr, _Token __token, _Alloc __alloc)
      : __alloc_(std::move(__alloc)),
        __assoc_(__token.try_associate()),
        __child_(execution::connect(std::forward<_Sndr>(__sndr), __rcvr_t(this))) {}

  _LIBCPP_HIDE_FROM_ABI void __start() noexcept {
    if (__assoc_)
      execution::start(__child_);
    else
      __on_complete();
  }

private:
  _LIBCPP_HIDE_FROM_ABI void __on_complete() noexcept override {
    auto __assoc      = std::move(__assoc_);
    using __rebound_t = typename allocator_traits<_Alloc>::template rebind_alloc<__spawn_op>;
    __rebound_t __a(__alloc_);
    this->~__spawn_op();
    allocator_traits<__rebound_t>::deallocate(__a, this, 1);
  }

  _Alloc __alloc_;
  decltype(std::declval<_Token>().try_associate()) __assoc_;
  connect_result_t<_Sndr, __rcvr_t> __child_;
};

// [exec.scope.spawn]: allocator selection -- from the explicit env argument, else the input
// sender's own environment, else allocator<byte>. Assumed fallback chain (see this facility's
// design note's "open item"): verify the exact order against the adopted text before treating
// this as final; the shape mirrors get_allocator's own established use elsewhere (P3433R1).
template <class _Sndr, class _Env>
_LIBCPP_HIDE_FROM_ABI constexpr auto __spawn_select_allocator(const _Sndr& __sndr, const _Env& __env) {
  if constexpr (requires { std::get_allocator(__env); }) {
    return std::get_allocator(__env);
  } else if constexpr (requires { std::get_allocator(execution::get_env(__sndr)); }) {
    return std::get_allocator(execution::get_env(__sndr));
  } else {
    return allocator<byte>();
  }
}

struct spawn_t {
  template <sender _Sndr, scope_token _Token, __queryable _Env = env<>>
  _LIBCPP_HIDE_FROM_ABI void operator()(_Sndr&& __sndr, _Token __token, _Env __env = {}) const {
    auto __wrapped = __token.wrap(std::forward<_Sndr>(__sndr));
    auto __alloc   = execution::__spawn_select_allocator(__wrapped, __env);
    auto __senv    = [&] {
      if constexpr (
          !requires { std::get_allocator(__env); } && requires { std::get_allocator(execution::get_env(__wrapped)); })
        return env(prop(std::get_allocator, __alloc), std::move(__env));
      else
        return std::move(__env);
    }();
    auto __in_sndr      = execution::write_env(std::move(__wrapped), std::move(__senv));
    using __alloc_t   = decltype(__alloc);
    using __op_t      = __spawn_op<decltype(__in_sndr), _Token, __alloc_t>;
    using __rebound_t = typename allocator_traits<__alloc_t>::template rebind_alloc<__op_t>;
    __rebound_t __a(__alloc);
    __op_t* __op = allocator_traits<__rebound_t>::allocate(__a, 1);
    try {
      ::new (static_cast<void*>(__op)) __op_t(std::move(__in_sndr), __token, __alloc);
    } catch (...) {
      allocator_traits<__rebound_t>::deallocate(__a, __op, 1);
      throw;
    }
    __op->__start();
  }
};

inline constexpr spawn_t spawn{};

// [exec.spawn.future] (Pass 3, P3149R11): spawn_future attempts to associate the given input
// sender with the given token's async scope and, on success, eagerly starts the input sender;
// the returned sender, when connected and started, completes with either the result of the
// eagerly-started input sender or with set_stopped if the input sender was never started
// (association failed). Unlike spawn(), the caller retains a handle to the outcome and may
// abandon it (destroy the returned sender before connecting/starting it) -- which sends a stop
// request to the still-running child rather than silently detaching it, matching this facility's
// entire "no detached work" philosophy from spawn() itself.
//
// [exec.spawn.future]p8-12 requires complete()/consume()/abandon() to "behave as atomic
// operations" appearing "to occur in a single total order" -- i.e. whichever of {complete,
// consume, abandon} happens first on a given spawn-future-state determines what the others do.
// Implemented with a plain mutex (matching simple_counting_scope's own established idiom for a
// structurally similar race in Pass 1, not lock-free atomics -- this isn't a per-index hot path
// the way parallel_scheduler's bulk dispatch is).
//
// [exec.stop.when]'s stop-when gets applied TWICE here, independently: once inside
// token.wrap(sndr) (this scope's own stop source, Pass 2), and again inside this state's own
// constructor using its own private inplace_stop_source (so abandoning *this particular future*
// requests stop without requesting stop on every other operation associated with the scope).
// <__execution/stop_when.h> supports this nesting with no changes needed -- each application
// independently combines whatever receiver-side token it sees with its own given token.
template <class _Sig>
struct __spawn_future_sig_args_nothrow;
template <class _Tag, class... _Args>
struct __spawn_future_sig_args_nothrow<_Tag(_Args...)> {
  static constexpr bool value = (is_nothrow_constructible_v<decay_t<_Args>, _Args> && ...);
};

template <class _Sig>
struct __spawn_future_as_tuple;
template <class _Tag, class... _Args>
struct __spawn_future_as_tuple<_Tag(_Args...)> {
  using type = __decayed_tuple<_Tag, _Args...>;
};

template <class _List>
struct __spawn_future_to_variant;
template <class... _Ts>
struct __spawn_future_to_variant<type_list<_Ts...>> {
  using type = variant<_Ts...>;
};

template <class _List>
struct __spawn_future_to_sigs;
template <class... _Sigs>
struct __spawn_future_to_sigs<type_list<_Sigs...>> {
  using type = completion_signatures<_Sigs...>;
};

// [exec.spawn.future]p3-4: the result variant this state's receiver stores into. monostate
// means "not yet completed"; tuple<set_stopped_t> covers both a genuine child set_stopped() and
// the constructor's own try_associate()-failed path; an extra tuple<set_error_t, exception_ptr>
// alternative is added only if some completion's arguments aren't unconditionally
// nothrow-decay-constructible (matching set-complete's own try/catch fallback below).
template <class _Completions>
struct __spawn_future_variant_for;
template <class... _Sigs>
struct __spawn_future_variant_for<completion_signatures<_Sigs...>> {
  static constexpr bool __all_nothrow = (__spawn_future_sig_args_nothrow<_Sigs>::value && ...);
  using type                          = __conditional_t<
      __all_nothrow,
      typename __spawn_future_to_variant<
          __dedup_type_list_t< monostate, tuple<set_stopped_t>, typename __spawn_future_as_tuple<_Sigs>::type...>>::
          type,
      typename __spawn_future_to_variant< __dedup_type_list_t<monostate,
                                                              tuple<set_stopped_t>,
                                                              tuple<set_error_t, exception_ptr>,
                                                              typename __spawn_future_as_tuple<_Sigs>::type...>>::type>;
};
template <class _Completions>
using __spawn_future_variant_t = typename __spawn_future_variant_for<_Completions>::type;

// The *outer* sender's own advertised completions: sigs-t widened with set_stopped_t() (always
// reachable, independent of the child: the try_associate()-failed path in the constructor) and,
// under the same condition __spawn_future_variant_for uses, set_error_t(exception_ptr) (the
// set-complete fallback below can produce it even if no completion signature in sigs-t itself
// ever names it). Must stay derived from the exact same predicate as the variant above -- these
// two computations are not allowed to drift from each other.
template <class _Completions>
struct __spawn_future_outer_sigs_for;
template <class... _Sigs>
struct __spawn_future_outer_sigs_for<completion_signatures<_Sigs...>> {
  static constexpr bool __all_nothrow = (__spawn_future_sig_args_nothrow<_Sigs>::value && ...);
  using type =
      __conditional_t< __all_nothrow,
                       typename __spawn_future_to_sigs<__dedup_type_list_t<_Sigs..., set_stopped_t()>>::type,
                       typename __spawn_future_to_sigs<
                           __dedup_type_list_t<_Sigs..., set_stopped_t(), set_error_t(exception_ptr)>>::type>;
};
template <class _Completions>
using __spawn_future_outer_sigs_t = typename __spawn_future_outer_sigs_for<_Completions>::type;

// [exec.spawn.future]p2: try-cancelable.
struct __try_cancelable {
  _LIBCPP_HIDE_FROM_ABI virtual void __try_cancel() noexcept = 0;

protected:
  _LIBCPP_HIDE_FROM_ABI ~__try_cancelable() = default;
};

// [exec.spawn.future]p3: spawn-future-state-base. Owns the result storage so the receiver (which
// only needs to populate it and signal completion) doesn't need to know the concrete state type,
// only this base -- mirrors __spawn_op_base's own role in spawn() above.
template <class _Completions>
struct __spawn_future_state_base : __try_cancelable {
  __spawn_future_variant_t<_Completions> __result;
  _LIBCPP_HIDE_FROM_ABI virtual void __complete() noexcept = 0;

protected:
  _LIBCPP_HIDE_FROM_ABI ~__spawn_future_state_base() = default;
};

// The virtual sink a registered consume() call completes through once complete() or
// try-set-stopped() eventually fires -- the state doesn't know the concrete receiver type, only
// this. Both operations first deregister the stop callback of the receiver, then complete it.
template <class _Completions>
struct __spawn_future_consume_sink {
  _LIBCPP_HIDE_FROM_ABI virtual void __on_complete() noexcept = 0;
  _LIBCPP_HIDE_FROM_ABI virtual void __on_stopped() noexcept  = 0;

protected:
  _LIBCPP_HIDE_FROM_ABI ~__spawn_future_consume_sink() = default;
};

// [exec.spawn.future]p5: spawn-future-receiver.
template <class _Completions>
class __spawn_future_receiver {
public:
  using receiver_concept = receiver_tag;

  __spawn_future_state_base<_Completions>* __state;

  template <class... _Args>
  _LIBCPP_HIDE_FROM_ABI void set_value(_Args&&... __args) && noexcept {
    __set_complete<set_value_t>(std::forward<_Args>(__args)...);
  }
  template <class _Err>
  _LIBCPP_HIDE_FROM_ABI void set_error(_Err&& __err) && noexcept {
    __set_complete<set_error_t>(std::forward<_Err>(__err));
  }
  _LIBCPP_HIDE_FROM_ABI void set_stopped() && noexcept { __set_complete<set_stopped_t>(); }

  _LIBCPP_HIDE_FROM_ABI auto get_env() const noexcept { return env<>{}; }

private:
  template <class _Cpo, class... _Args>
  _LIBCPP_HIDE_FROM_ABI void __set_complete(_Args&&... __args) noexcept {
    constexpr bool __nothrow = (is_nothrow_constructible_v<decay_t<_Args>, _Args> && ...);
    try {
      __state->__result.template emplace<__decayed_tuple<_Cpo, _Args...>>(_Cpo{}, std::forward<_Args>(__args)...);
    } catch (...) {
      if constexpr (!__nothrow) {
        using __tuple_t = __decayed_tuple<set_error_t, exception_ptr>;
        __state->__result.template emplace<__tuple_t>(set_error_t{}, std::current_exception());
      }
    }
    __state->__complete();
  }
};

template <class _State>
struct __spawn_future_deleter {
  // [exec.spawn.future]p16.2: "u.get_deleter()(u.release()) is equivalent to
  // u.release()->abandon()" -- the deleter's effect is abandon(), never a direct delete: abandon()
  // itself decides whether that means requesting stop on a still-running operation or tearing
  // down an already-finished one (see __spawn_future_state::__abandon below).
  _LIBCPP_HIDE_FROM_ABI void operator()(_State* __p) const noexcept { __p->__abandon(); }
};

// [exec.spawn.future]p6-7: spawn-future-state. _Sndr here is already token.wrap(original_sndr)
// (computed once by spawn_future_t::operator() and reused, not re-evaluated) -- this class
// applies stop-when a second time, independently, via its own private stop source.
template <class _Alloc, class _Token, class _Sndr, class _Env>
class __spawn_future_state final
    : public __spawn_future_state_base<completion_signatures_of_t<
          decltype(execution::write_env(
              execution::__stop_when(std::declval<_Sndr>(), std::declval<inplace_stop_token>()), std::declval<_Env>())),
          env<>>> {
  using __wrapped_t = decltype(execution::write_env(
      execution::__stop_when(std::declval<_Sndr>(), std::declval<inplace_stop_token>()), std::declval<_Env>()));

public:
  using __sigs_t  = completion_signatures_of_t<__wrapped_t, env<>>;
  using __rcvr_t  = __spawn_future_receiver<__sigs_t>;
  using __alloc_t = typename allocator_traits<_Alloc>::template rebind_alloc<__spawn_future_state>;

  _LIBCPP_HIDE_FROM_ABI __spawn_future_state(_Alloc __alloc, _Sndr&& __sndr, _Token __token, _Env __env)
      : __alloc_(std::move(__alloc)),
        __op_(execution::connect(
            execution::write_env(
                execution::__stop_when(std::forward<_Sndr>(__sndr), __ssource_.get_token()), std::move(__env)),
            __rcvr_t{this})),
        __associated_(__token.try_associate()) {
    if (__associated_) {
      execution::start(__op_);
    } else {
      execution::set_stopped(__rcvr_t{this});
    }
  }

  __spawn_future_state(const __spawn_future_state&)            = delete;
  __spawn_future_state& operator=(const __spawn_future_state&) = delete;

  // [exec.spawn.future]p9. Called from __rcvr_t's set_complete, i.e. whenever the wrapped
  // sender's operation finishes -- possibly synchronously, nested inside a request_stop() call of
  // __try_cancel()/__abandon() (a callback firing inline), or possibly much later and fully
  // asynchronously. The state is destroyed once the operation has finished *and* nothing is left
  // to tell: a registered receiver is completed with the result first.
  _LIBCPP_HIDE_FROM_ABI void __complete() noexcept override {
    __spawn_future_consume_sink<__sigs_t>* __to_call = nullptr;
    bool __do_destroy                                = false;
    {
      lock_guard<mutex> __lock(__mtx_);
      __completed_ = true;
      if (__registered_) {
        __to_call        = std::exchange(__registered_, nullptr);
        __receiver_done_  = true;
      } else if (__receiver_done_ || __abandoned_) {
        __do_destroy = true;
      }
      // Otherwise the result is stored until consume() (or abandon()) arrives.
    }
    if (__to_call) {
      __to_call->__on_complete();
      __request_destroy();
    } else if (__do_destroy) {
      __request_destroy();
    }
  }

  // [exec.spawn.future]p10. Called from the outer opstate's start(), which has handed over its
  // ownership of *this (it is destroyed by complete(), not by the opstate).
  _LIBCPP_HIDE_FROM_ABI void __consume(__spawn_future_consume_sink<__sigs_t>* __sink) noexcept {
    bool __deliver_result = false;
    bool __deliver_stop   = false;
    {
      lock_guard<mutex> __lock(__mtx_);
      __consumed_ = true;
      if (__completed_) {
        __deliver_result = true;
        __receiver_done_ = true;
      } else if (__stop_pending_) {
        __deliver_stop   = true;
        __receiver_done_ = true;
      } else {
        __registered_ = __sink;
      }
    }
    if (__deliver_result) {
      __sink->__on_complete();
      __request_destroy();
    } else if (__deliver_stop) {
      // The operation has not finished: complete() destroys the state when it does.
      __sink->__on_stopped();
    }
  }

  // [exec.spawn.future]p11: stop requested through the consuming receiver's stop token.
  _LIBCPP_HIDE_FROM_ABI void __try_cancel() noexcept override {
    {
      lock_guard<mutex> __lock(__mtx_);
      ++__busy_;
    }
    __ssource_.request_stop();
    __try_set_stopped();
    __release_busy();
  }

  // [exec.spawn.future]p12. Deviation from the literal wording: the draft's try-set-stopped also invokes destroy(), which
  // would destroy a state whose wrapped operation may still be running (a member), and complete()'s last bullet would
  // then destroy it a second time. Here the state is destroyed once, by complete(), when the operation has finished.
  // (No existing LWG issue found for this.)
  _LIBCPP_HIDE_FROM_ABI void __try_set_stopped() noexcept {
    __spawn_future_consume_sink<__sigs_t>* __to_call = nullptr;
    {
      lock_guard<mutex> __lock(__mtx_);
      if (__registered_) {
        __to_call        = std::exchange(__registered_, nullptr);
        __receiver_done_ = true;
        __stop_pending_  = true;
      } else if (!__consumed_ && !__completed_) {
        __stop_pending_ = true; // consume() will complete the receiver with set_stopped
      }
    }
    if (__to_call) {
      __to_call->__on_stopped();
    }
  }

  // [exec.spawn.future]p13, called from __spawn_future_deleter when the state was never consumed.
  _LIBCPP_HIDE_FROM_ABI void __abandon() noexcept {
    bool __destroy_now = false;
    {
      lock_guard<mutex> __lock(__mtx_);
      __abandoned_ = true;
      if (__completed_) {
        __destroy_now = true;
      } else {
        ++__busy_;
      }
    }
    if (__destroy_now) {
      __request_destroy();
    } else {
      __ssource_.request_stop();
      __release_busy();
    }
  }

private:
  // Destroy now unless a request_stop() of __try_cancel()/__abandon() is still on the stack
  // (it would run on a destroyed __ssource_): then the last of those destroys.
  _LIBCPP_HIDE_FROM_ABI void __request_destroy() noexcept {
    {
      lock_guard<mutex> __lock(__mtx_);
      if (__busy_ != 0) {
        __destroy_pending_ = true;
        return;
      }
    }
    __destroy();
  }

  _LIBCPP_HIDE_FROM_ABI void __release_busy() noexcept {
    bool __destroy_now;
    {
      lock_guard<mutex> __lock(__mtx_);
      --__busy_;
      __destroy_now = __busy_ == 0 && __destroy_pending_;
    }
    if (__destroy_now) {
      __destroy();
    }
  }

  // [exec.spawn.future]p14.
  _LIBCPP_HIDE_FROM_ABI void __destroy() noexcept {
    auto __associated = std::move(__associated_);
    __alloc_t __a(__alloc_);
    allocator_traits<__alloc_t>::destroy(__a, this);
    allocator_traits<__alloc_t>::deallocate(__a, this, 1);
  }

  _Alloc __alloc_;
  inplace_stop_source __ssource_;
  connect_result_t<__wrapped_t, __rcvr_t> __op_;
  decltype(std::declval<_Token>().try_associate()) __associated_;
  mutex __mtx_;
  __spawn_future_consume_sink<__sigs_t>* __registered_ = nullptr;
  unsigned __busy_                                     = 0;
  bool __completed_                                    = false; // the wrapped operation has finished
  bool __consumed_                                     = false; // consume() was called
  bool __receiver_done_                                = false; // the receiver was completed (result or stopped)
  bool __stop_pending_                                 = false; // stop was requested before consume()
  bool __abandoned_                                    = false;
  bool __destroy_pending_                              = false;
};

// [exec.spawn.future]p15: the outer sender's own opstate. Owns the unique_ptr until start() (so the
// state is abandon()ed -- not directly deleted -- if this opstate is destroyed before start()), then
// hands the state over to itself: from then on the state destroys itself once the operation has
// finished. Doubles as the consume-sink the state completes the receiver through, and owns the
// registration of the receiver's stop callback (a stop request cancels the future: try-cancel).
template <class _State, class _Rcvr>
class __spawn_future_opstate final : public __spawn_future_consume_sink<typename _State::__sigs_t> {
  struct __callback {
    __try_cancelable* __state_;
    _LIBCPP_HIDE_FROM_ABI void operator()() noexcept { __state_->__try_cancel(); }
  };
  using __stop_token_t    = stop_token_of_t<env_of_t<_Rcvr>>;
  using __stop_callback_t = stop_callback_for_t<__stop_token_t, __callback>;

public:
  using operation_state_concept = operation_state_tag;

  _LIBCPP_HIDE_FROM_ABI
  __spawn_future_opstate(unique_ptr<_State, __spawn_future_deleter<_State>> __state, _Rcvr&& __rcvr)
      : __state_(std::move(__state)), __raw_(__state_.get()), __rcvr_(std::move(__rcvr)) {}

  __spawn_future_opstate(const __spawn_future_opstate&)            = delete;
  __spawn_future_opstate& operator=(const __spawn_future_opstate&) = delete;

  _LIBCPP_HIDE_FROM_ABI void start() & noexcept {
    constexpr bool __nothrow = is_nothrow_constructible_v<__stop_callback_t, __stop_token_t, __callback>;
    try {
      // The draft writes get_stop_token(rcvr) here, which is ill-formed against its own stop-token-t =
      // stop_token_of_t<env_of_t<Rcvr>> for a stoppable token; the environment's token is meant (unlike
      // run_loop's literal get_stop_token(REC(o)), which compiles and is kept as written). Like the draft, a throwing
      // callback construction completes with set_error(exception_ptr) even though the signatures only advertise it
      // for throwing datums (dead code for the standard stop tokens).
      __stop_cb_.emplace(std::get_stop_token(execution::get_env(__rcvr_)), __callback{__raw_});
    } catch (...) {
      if constexpr (!__nothrow) {
        execution::set_error(std::move(__rcvr_), std::current_exception());
        return;
      }
    }
    __state_.release()->__consume(this);
  }

private:
  _LIBCPP_HIDE_FROM_ABI void __on_complete() noexcept override {
    __stop_cb_.reset();
    std::move(__raw_->__result).visit([this](auto&& __tup) noexcept {
      if constexpr (!same_as<remove_cvref_t<decltype(__tup)>, monostate>) {
        std::apply([this](auto __cpo,
                          auto&&... __vals) { __cpo(std::move(__rcvr_), std::forward<decltype(__vals)>(__vals)...); },
                   std::forward<decltype(__tup)>(__tup));
      }
    });
  }

  _LIBCPP_HIDE_FROM_ABI void __on_stopped() noexcept override {
    __stop_cb_.reset();
    execution::set_stopped(std::move(__rcvr_));
  }

  unique_ptr<_State, __spawn_future_deleter<_State>> __state_;
  _State* __raw_;
  _Rcvr __rcvr_;
  optional<__stop_callback_t> __stop_cb_;
};

// [exec.spawn.future]p16.3: make-sender(spawn_future, std::move(u)) -- a leaf sender (no child:
// the input sender was already consumed into the eagerly-started state above), holding only the
// unique_ptr. Not routed through the draft's basic-sender/impls-for/make-sender machinery, same
// M3 deviation as every other hand-rolled sender in this fork.
//
// spawn_future_t is defined in full here, before __spawn_future_sndr, since the latter's `tag`
// member names it non-dependently -- same ordering constraint (and reason) as
// <__execution/write_env.h>'s __write_env_t/__write_env_sndr pair: spawn_future_t::operator()'s
// own body references __spawn_future_sndr, but only inside a template body (deferred until
// instantiation, by which point __spawn_future_sndr's own definition below is complete).
template <class _State>
class __spawn_future_sndr;

struct spawn_future_t {
  template <sender _Sndr, scope_token _Token, __queryable _Env = env<>>
  _LIBCPP_HIDE_FROM_ABI auto operator()(_Sndr&& __sndr, _Token __token, _Env __env = {}) const {
    auto __new_sender    = __token.wrap(std::forward<_Sndr>(__sndr));
    using __new_sender_t = decltype(__new_sender);
    auto __alloc         = execution::__spawn_select_allocator(__new_sender, __env);
    using __alloc_t      = decltype(__alloc);
    using __state_t      = __spawn_future_state<__alloc_t, _Token, __new_sender_t, _Env>;
    using __rebound_t    = typename allocator_traits<__alloc_t>::template rebind_alloc<__state_t>;

    __rebound_t __a(__alloc);
    __state_t* __raw = allocator_traits<__rebound_t>::allocate(__a, 1);
    try {
      ::new (static_cast<void*>(__raw)) __state_t(__alloc, std::move(__new_sender), __token, std::move(__env));
    } catch (...) {
      allocator_traits<__rebound_t>::deallocate(__a, __raw, 1);
      throw;
    }
    return __spawn_future_sndr<__state_t>{{}, unique_ptr<__state_t, __spawn_future_deleter<__state_t>>(__raw)};
  }
};

inline constexpr spawn_future_t spawn_future{};

template <class _State>
class __spawn_future_sndr {
public:
  using sender_concept = sender_tag;

  _LIBCPP_NO_UNIQUE_ADDRESS spawn_future_t tag;
  unique_ptr<_State, __spawn_future_deleter<_State>> data;

  template <class _Rcvr>
  _LIBCPP_HIDE_FROM_ABI auto connect(_Rcvr&& __rcvr) && {
    return __spawn_future_opstate<_State, remove_cvref_t<_Rcvr>>(std::move(data), std::forward<_Rcvr>(__rcvr));
  }

  _LIBCPP_HIDE_FROM_ABI auto get_env() const noexcept { return env<>{}; }

  // The completion signatures were already fixed the moment spawn_future(sndr, token, env) was
  // called (Env is baked into _State's own type) -- unlike an ordinary adaptor, they don't
  // additionally depend on whatever environment this sender is eventually connected through.
  template <class _Self, class... _Env>
  _LIBCPP_HIDE_FROM_ABI static consteval auto get_completion_signatures() {
    return __spawn_future_outer_sigs_t<typename _State::__sigs_t>{};
  }
};

} // namespace execution

#endif // _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_THREADS

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP___EXECUTION_ASYNC_SCOPE_H
