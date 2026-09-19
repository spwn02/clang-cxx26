//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___EXECUTION_ASYNC_SCOPE_H
#define _LIBCPP___EXECUTION_ASYNC_SCOPE_H

#include <__concepts/copyable.h>
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
#include <__execution/operation_state.h>
#include <__execution/receiver.h>
#include <__execution/sender.h>
#include <__execution/stop_when.h>
#include <__memory/allocator.h>
#include <__memory/allocator_traits.h>
#include <__memory/unique_ptr.h>
#include <__mutex/lock_guard.h>
#include <__mutex/mutex.h>
#include <__stop_token/inplace_stop_source.h>
#include <__stop_token/inplace_stop_token.h>
#include <__type_traits/conditional.h>
#include <__type_traits/is_nothrow_constructible.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/forward.h>
#include <__utility/move.h>
#include <cstddef>
#include <exception>
#include <tuple>
#include <variant>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_PUSH_MACROS
#include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

// execution::async_scope (P3149R11). See docs/design/async_scope_p3149.md for the design
// note this implementation follows, including the staging plan: this file implements Pass 1
// (scope_token, simple_counting_scope, associate, spawn, join) and Pass 2 (counting_scope) --
// spawn_future (Pass 3) is explicitly deferred, tracked as follow-up work, not silently
// dropped.
#if _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_THREADS

namespace execution {

// [exec.scope.token] (simplified: the paper's own concept additionally checks that
// tok.wrap(s) is well-formed for an exposition-only test-sender/test-env; building that
// exact machinery is out of scope for this pass -- a token type missing a usable wrap()
// still fails to compile the moment associate()/spawn() actually calls it, just later than
// this concept-check would catch it).
template <class _Token>
concept scope_token = copyable<_Token> && requires(const _Token& __tok) {
  { __tok.try_associate() } -> same_as<bool>;
  { __tok.disassociate() } noexcept;
};

// [exec.scope.simple.counting]: the association-count/state machinery shared by
// simple_counting_scope's token and (once Pass 2 lands) counting_scope's token.
class simple_counting_scope {
public:
  class token {
  public:
    template <sender _Sndr>
    _LIBCPP_HIDE_FROM_ABI _Sndr&& wrap(_Sndr&& __sndr) const noexcept {
      return std::forward<_Sndr>(__sndr);
    }
    _LIBCPP_HIDE_FROM_ABI bool try_associate() const noexcept { return __scope_->__try_associate(); }
    _LIBCPP_HIDE_FROM_ABI void disassociate() const noexcept { __scope_->__disassociate(); }

  private:
    friend class simple_counting_scope;
    _LIBCPP_HIDE_FROM_ABI explicit token(simple_counting_scope* __scope) noexcept : __scope_(__scope) {}
    simple_counting_scope* __scope_;
  };

  _LIBCPP_HIDE_FROM_ABI simple_counting_scope() noexcept = default;

  simple_counting_scope(simple_counting_scope&&)            = delete;
  simple_counting_scope& operator=(simple_counting_scope&&) = delete;

  // [exec.scope.simple.counting]p2: terminate() unless in the unused, unused-and-closed, or
  // joined state -- all three are exactly "no outstanding associations and no pending
  // joiner", i.e. __count_ == 0 && __sink_ == nullptr (a joiner that already completed
  // synchronously, or was never registered, always leaves __sink_ null; one still
  // genuinely waiting never gets past this check while __count_ could still reach 0).
  _LIBCPP_HIDE_FROM_ABI ~simple_counting_scope() {
    if (__count_ != 0 || __sink_ != nullptr) {
      std::terminate();
    }
  }

  _LIBCPP_HIDE_FROM_ABI token get_token() noexcept { return token(this); }

  _LIBCPP_HIDE_FROM_ABI void close() noexcept {
    lock_guard<mutex> __lock(__mtx_);
    __closed_ = true;
  }

  _LIBCPP_HIDE_FROM_ABI auto join() noexcept;

private:
  struct __join_sink_base {
    _LIBCPP_HIDE_FROM_ABI virtual void __complete() noexcept = 0;

  protected:
    _LIBCPP_HIDE_FROM_ABI ~__join_sink_base() = default;
  };

  template <class _Rcvr>
  class __join_opstate;
  class __join_sender;

  _LIBCPP_HIDE_FROM_ABI bool __try_associate() noexcept {
    lock_guard<mutex> __lock(__mtx_);
    if (__closed_) {
      return false;
    }
    ++__count_;
    return true;
  }

  _LIBCPP_HIDE_FROM_ABI void __disassociate() noexcept {
    __join_sink_base* __to_complete = nullptr;
    {
      lock_guard<mutex> __lock(__mtx_);
      --__count_;
      if (__count_ == 0 && __sink_ != nullptr) {
        __to_complete = __sink_;
        __sink_       = nullptr;
      }
    }
    if (__to_complete != nullptr) {
      __to_complete->__complete();
    }
  }

  // Called from __join_opstate::start(): either completes inline immediately (the
  // synchronous fast path [exec.scope.simple.counting]p9 requires when the count is already
  // zero) or registers __sink for whichever disassociate() call brings the count to zero.
  _LIBCPP_HIDE_FROM_ABI void __join_start(__join_sink_base* __sink) noexcept {
    bool __complete_now;
    {
      lock_guard<mutex> __lock(__mtx_);
      __complete_now = (__count_ == 0);
      if (!__complete_now) {
        __sink_ = __sink;
      }
    }
    if (__complete_now) {
      __sink->__complete();
    }
  }

  mutex __mtx_;
  size_t __count_       = 0;
  bool __closed_        = false;
  __join_sink_base* __sink_ = nullptr;
};

template <class _Rcvr>
class simple_counting_scope::__join_opstate final : public simple_counting_scope::__join_sink_base {
public:
  using operation_state_concept = operation_state_tag;

  _LIBCPP_HIDE_FROM_ABI __join_opstate(simple_counting_scope* __scope, _Rcvr&& __rcvr) noexcept
      : __scope_(__scope), __rcvr_(std::move(__rcvr)) {}

  __join_opstate(const __join_opstate&)            = delete;
  __join_opstate& operator=(const __join_opstate&) = delete;

  _LIBCPP_HIDE_FROM_ABI void start() & noexcept { __scope_->__join_start(this); }

private:
  _LIBCPP_HIDE_FROM_ABI void __complete() noexcept override { execution::set_value(std::move(__rcvr_)); }

  simple_counting_scope* __scope_;
  _Rcvr __rcvr_;
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

  template <class _Self, class _Env>
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
    _LIBCPP_HIDE_FROM_ABI bool try_associate() const noexcept {
      return __scope_->__scope_.get_token().try_associate();
    }
    _LIBCPP_HIDE_FROM_ABI void disassociate() const noexcept { __scope_->__scope_.get_token().disassociate(); }

  private:
    friend class counting_scope;
    _LIBCPP_HIDE_FROM_ABI explicit token(counting_scope* __scope) noexcept : __scope_(__scope) {}
    counting_scope* __scope_;
  };

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
  simple_counting_scope __scope_;
  inplace_stop_source __source_;
};

// [exec.scope.associate]. A basis operation, not composed from spawn (nor vice versa): wraps
// the input sender via the token, then on start() either try_associate()s and connects+starts
// the wrapped sender (forwarding its completions unchanged), or -- if try_associate() fails
// (the scope is closed) -- completes with set_stopped() directly, never running the wrapped
// sender at all. The association is released from the operation state's destructor exactly
// once, regardless of how the wrapped sender completes (an RAII property, per the paper).
template <class _Sndr, class _Token, class _Rcvr>
class __associate_opstate;

// Stores a direct `_Rcvr&` (not reached through `__state_`) for get_env(), matching
// <__execution/continues_on.h>'s own documented incomplete-type fix: computing
// `connect_result_t<_Sndr, __inner_rcvr_t>` inside the still-incomplete __associate_opstate
// transitively *calls* this receiver's get_env() body as part of forming that type, which a
// body reaching back through `__state_` can't do yet. set_value()/set_error()/set_stopped()
// (which do need `__state_`, to reach the token for a future extension point) are ordinary
// non-template-instantiated-later member functions, not subject to the same constraint.
template <class _Sndr, class _Token, class _Rcvr>
class __associate_inner_rcvr {
public:
  using receiver_concept = receiver_tag;

  _LIBCPP_HIDE_FROM_ABI explicit __associate_inner_rcvr(__associate_opstate<_Sndr, _Token, _Rcvr>* __state,
                                                          _Rcvr& __rcvr) noexcept
      : __state_(__state), __rcvr_(__rcvr) {}

  template <class... _Args>
  _LIBCPP_HIDE_FROM_ABI void set_value(_Args&&... __args) && noexcept {
    execution::set_value(std::move(__rcvr_), std::forward<_Args>(__args)...);
  }
  template <class _Err>
  _LIBCPP_HIDE_FROM_ABI void set_error(_Err&& __err) && noexcept {
    execution::set_error(std::move(__rcvr_), std::forward<_Err>(__err));
  }
  _LIBCPP_HIDE_FROM_ABI void set_stopped() && noexcept { execution::set_stopped(std::move(__rcvr_)); }

  _LIBCPP_HIDE_FROM_ABI auto get_env() const noexcept { return execution::__fwd_env_fn(execution::get_env(__rcvr_)); }

private:
  __associate_opstate<_Sndr, _Token, _Rcvr>* __state_;
  _Rcvr& __rcvr_;
};

template <class _Sndr, class _Token, class _Rcvr>
class __associate_opstate {
  using __inner_rcvr_t = __associate_inner_rcvr<_Sndr, _Token, _Rcvr>;
  using __child_op_t   = connect_result_t<_Sndr, __inner_rcvr_t>;

public:
  using operation_state_concept = operation_state_tag;

  // Connects the (already-wrapped) child sender unconditionally here, in the member-
  // initializer-list -- not lazily in start() once try_associate() is known to succeed. This
  // looks like it front-runs the paper's own "try_associate(), then if successful connect"
  // ordering, but connect() itself is required to be side-effect-free for any conforming
  // sender (all real work happens in start()), so connecting a child that never gets
  // start()ed is unobservable. The alternative -- an `optional<__child_op_t>` populated via
  // `emplace()` inside start() -- needs __child_op_t to be move-constructible (emplace binds
  // its argument through a forwarding-reference parameter, which is not a guaranteed-elision
  // context), and most hand-written operation states in this fork (e.g. __just_opstate) are
  // deliberately not movable, relying on exactly this kind of direct member-initializer
  // construction instead.
  _LIBCPP_HIDE_FROM_ABI __associate_opstate(_Sndr&& __sndr, _Token __token, _Rcvr&& __rcvr)
      : __token_(std::move(__token)), __rcvr_(std::move(__rcvr)),
        __child_(execution::connect(std::forward<_Sndr>(__sndr), __inner_rcvr_t(this, __rcvr_))) {}

  __associate_opstate(const __associate_opstate&)            = delete;
  __associate_opstate& operator=(const __associate_opstate&) = delete;

  _LIBCPP_HIDE_FROM_ABI ~__associate_opstate() {
    if (__associated_) {
      __token_.disassociate();
    }
  }

  _LIBCPP_HIDE_FROM_ABI void start() & noexcept {
    if (__token_.try_associate()) {
      __associated_ = true;
      execution::start(__child_);
    } else {
      execution::set_stopped(std::move(__rcvr_));
    }
  }

private:
  friend class __associate_inner_rcvr<_Sndr, _Token, _Rcvr>;

  _Token __token_;
  _Rcvr __rcvr_;
  bool __associated_ = false;
  __child_op_t __child_;
};

template <class _Tag, class _Token, class _Sndr>
class __associate_sndr {
public:
  using sender_concept = sender_tag;

  _LIBCPP_NO_UNIQUE_ADDRESS _Tag tag;
  _Token token;
  _Sndr child;

  template <class _Rcvr>
  _LIBCPP_HIDE_FROM_ABI auto connect(_Rcvr&& __rcvr) && -> __associate_opstate<_Sndr, _Token, remove_cvref_t<_Rcvr>> {
    return __associate_opstate<_Sndr, _Token, remove_cvref_t<_Rcvr>>(
        std::move(child), std::move(token), std::forward<_Rcvr>(__rcvr));
  }

  _LIBCPP_HIDE_FROM_ABI auto get_env() const noexcept { return execution::get_env(child); }

  template <class _Self, class _Env>
  _LIBCPP_HIDE_FROM_ABI static consteval auto get_completion_signatures() {
    return completion_signatures_of_t<_Sndr, _Env>{};
  }
};

struct associate_t {
  template <sender _Sndr, scope_token _Token>
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Sndr&& __sndr, _Token __token) const {
    using __wrapped_t = decltype(__token.wrap(std::forward<_Sndr>(__sndr)));
    return __associate_sndr<associate_t, _Token, __wrapped_t>{{}, std::move(__token), __token.wrap(std::forward<_Sndr>(__sndr))};
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
      : __token_(std::move(__token)), __alloc_(std::move(__alloc)),
        __child_(execution::connect(std::forward<_Sndr>(__sndr), __rcvr_t(this))) {}

  _LIBCPP_HIDE_FROM_ABI void __start() noexcept { execution::start(__child_); }

private:
  _LIBCPP_HIDE_FROM_ABI void __on_complete() noexcept override {
    _Token __token = std::move(__token_);
    using __rebound_t = typename allocator_traits<_Alloc>::template rebind_alloc<__spawn_op>;
    __rebound_t __a(__alloc_);
    this->~__spawn_op();
    allocator_traits<__rebound_t>::deallocate(__a, this, 1);
    __token.disassociate();
  }

  _Token __token_;
  _Alloc __alloc_;
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
    auto __wrapped         = __token.wrap(std::forward<_Sndr>(__sndr));
    using __wrapped_t       = decltype(__wrapped);
    auto __alloc            = execution::__spawn_select_allocator(__wrapped, __env);
    using __alloc_t          = decltype(__alloc);
    using __op_t             = __spawn_op<__wrapped_t, _Token, __alloc_t>;
    using __rebound_t        = typename allocator_traits<__alloc_t>::template rebind_alloc<__op_t>;

    // [exec.scope.spawn]p3: Mandates aside, a closed scope simply means this spawn is a no-op
    // -- there is no receiver to notify of failure (spawn returns void), matching "no detached
    // work by default": nothing was scheduled, nothing needs cleaning up.
    if (!__token.try_associate()) {
      return;
    }

    __rebound_t __a(__alloc);
    __op_t* __op = allocator_traits<__rebound_t>::allocate(__a, 1);
    try {
      ::new (static_cast<void*>(__op)) __op_t(std::move(__wrapped), __token, __alloc);
    } catch (...) {
      allocator_traits<__rebound_t>::deallocate(__a, __op, 1);
      __token.disassociate();
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
  using type = __conditional_t<
      __all_nothrow,
      typename __spawn_future_to_variant<__dedup_type_list_t<
          monostate, tuple<set_stopped_t>, typename __spawn_future_as_tuple<_Sigs>::type...>>::type,
      typename __spawn_future_to_variant<
          __dedup_type_list_t<monostate, tuple<set_stopped_t>, tuple<set_error_t, exception_ptr>,
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
  using type = __conditional_t<
      __all_nothrow, typename __spawn_future_to_sigs<__dedup_type_list_t<_Sigs..., set_stopped_t()>>::type,
      typename __spawn_future_to_sigs<
          __dedup_type_list_t<_Sigs..., set_stopped_t(), set_error_t(exception_ptr)>>::type>;
};
template <class _Completions>
using __spawn_future_outer_sigs_t = typename __spawn_future_outer_sigs_for<_Completions>::type;

// [exec.spawn.future]p3: spawn-future-state-base. Owns the result storage so the receiver (which
// only needs to populate it and signal completion) doesn't need to know the concrete state type,
// only this base -- mirrors __spawn_op_base's own role in spawn() above.
template <class _Completions>
struct __spawn_future_state_base {
  __spawn_future_variant_t<_Completions> __result;
  _LIBCPP_HIDE_FROM_ABI virtual void __complete() noexcept = 0;

protected:
  _LIBCPP_HIDE_FROM_ABI ~__spawn_future_state_base() = default;
};

// The virtual sink a registered consume() call completes through once complete() eventually
// fires -- the state doesn't know the concrete receiver type, only this. Same role as
// simple_counting_scope::__join_sink_base in Pass 1.
template <class _Completions>
struct __spawn_future_consume_sink {
  _LIBCPP_HIDE_FROM_ABI virtual void __on_complete() noexcept = 0;

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
          decltype(execution::write_env(execution::__stop_when(std::declval<_Sndr>(), std::declval<inplace_stop_token>()),
                                         std::declval<_Env>())),
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
            execution::write_env(execution::__stop_when(std::forward<_Sndr>(__sndr), __ssource_.get_token()),
                                  std::move(__env)),
            __rcvr_t{this})),
        __token_(std::move(__token)),
        __associated_(__token_.try_associate()) {
    if (__associated_) {
      execution::start(__op_);
    } else {
      execution::set_stopped(__rcvr_t{this});
    }
  }

  __spawn_future_state(const __spawn_future_state&)            = delete;
  __spawn_future_state& operator=(const __spawn_future_state&) = delete;

  // [exec.spawn.future]p9. Called from __rcvr_t's set_complete, i.e. whenever the wrapped
  // sender's operation finishes -- possibly synchronously, nested inside __abandon()'s own call
  // to request_stop() (a callback firing inline because the token was already stop-requested by
  // something else, or because the child reacts to the request without ever suspending), or
  // possibly much later and fully asynchronously (a genuinely deferred operation, e.g. scheduled
  // on a run_loop, completing only once something eventually drains it -- request_stop() itself
  // returned long ago in that case, its own call frame gone).
  _LIBCPP_HIDE_FROM_ABI void __complete() noexcept override {
    __spawn_future_consume_sink<__sigs_t>* __to_call = nullptr;
    bool __do_destroy                                = false;
    {
      lock_guard<mutex> __lock(__mtx_);
      switch (__phase_) {
      case __phase::__initial:
        __phase_ = __phase::__completed;
        break;
      case __phase::__consumed:
        // Transition to __completed even though the result is being dispatched immediately
        // below, not stored for a later consume() -- __completed is also "there is nothing left
        // to wait for," which is exactly what __abandon() needs to see later, once the outer
        // opstate (and the unique_ptr it owns) is eventually destroyed: __abandon()'s own
        // __completed branch is "just tear down," the correct action once the registered
        // receiver has already been (or is about to be) notified. Without this, __phase_ would
        // stay stuck at __consumed forever, __abandon() would silently no-op on it (its switch
        // has no __consumed case, since abandonment before consumption and after are the only
        // two states it's ever meant to observe), and the state -- along with its scope
        // association -- would never be destroyed at all.
        __phase_  = __phase::__completed;
        __to_call = __registered_;
        break;
      case __phase::__abandoned:
        if (__request_stop_in_progress_) {
          // Running synchronously inside __abandon()'s own still-unwinding call to
          // request_stop() -- destroying *this now would free __ssource_ out from under that
          // call. Defer: __abandon() re-checks this flag right after request_stop() returns.
          __complete_seen_while_abandoned_ = true;
        } else {
          // The ordinary case: request_stop() already fully returned (possibly long ago) on a
          // stack that's gone -- nothing is relying on __ssource_ staying alive, so it's safe to
          // tear down directly, right here, without waiting for anyone to re-check anything.
          __do_destroy = true;
        }
        break;
      default:
        break; // unreachable: complete() cannot fire twice, nor after __completed itself
      }
    }
    // Nothing below this point may touch any member if __to_call fires and its completion
    // (transitively) leads to *this being destroyed, nor after __destroy() -- both must be the
    // last thing this function does with `this`.
    if (__to_call) {
      __to_call->__on_complete();
    } else if (__do_destroy) {
      __destroy();
    }
  }

  // [exec.spawn.future]p10. Called from the outer sender's own opstate::start().
  _LIBCPP_HIDE_FROM_ABI void __consume(__spawn_future_consume_sink<__sigs_t>* __sink) noexcept {
    bool __call_now = false;
    {
      lock_guard<mutex> __lock(__mtx_);
      switch (__phase_) {
      case __phase::__initial:
        __phase_     = __phase::__consumed;
        __registered_ = __sink;
        break;
      case __phase::__completed:
        __call_now = true;
        break;
      default:
        break; // unreachable: consume() is only ever called once, by the one outer opstate
      }
    }
    if (__call_now) {
      __sink->__on_complete();
    }
  }

  // [exec.spawn.future]p11, called from __spawn_future_deleter -- never a direct delete.
  _LIBCPP_HIDE_FROM_ABI void __abandon() noexcept {
    bool __request_stop_needed = false;
    bool __destroy_now         = false;
    {
      lock_guard<mutex> __lock(__mtx_);
      switch (__phase_) {
      case __phase::__initial:
        __phase_             = __phase::__abandoned;
        __request_stop_needed = true;
        break;
      case __phase::__completed:
        __destroy_now = true;
        break;
      default:
        break; // unreachable: abandon() only ever fires via the unique_ptr's own single deleter
      }
    }
    if (__destroy_now) {
      __destroy();
      return;
    }
    if (__request_stop_needed) {
      {
        lock_guard<mutex> __lock(__mtx_);
        __request_stop_in_progress_ = true;
      }
      __ssource_.request_stop();
      // __complete() may have already fired synchronously, from within the call above, while
      // this function was inside it -- deferring the destroy decision here rather than racing
      // request_stop()'s own still-unwinding stack. Safe to act on that now: the flag flip below
      // happens-before any later, asynchronous __complete() could observe
      // __request_stop_in_progress_ as false and destroy directly itself instead (see
      // __complete()'s own __phase::__abandoned branch) -- exactly one of the two ever destroys.
      bool __destroy_after_stop;
      {
        lock_guard<mutex> __lock(__mtx_);
        __request_stop_in_progress_ = false;
        __destroy_after_stop        = __complete_seen_while_abandoned_;
      }
      if (__destroy_after_stop) {
        __destroy();
      }
    }
  }

private:
  // [exec.spawn.future]p12.
  _LIBCPP_HIDE_FROM_ABI void __destroy() noexcept {
    _Token __token         = std::move(__token_);
    bool __was_associated = __associated_;
    {
      __alloc_t __a(__alloc_);
      allocator_traits<__alloc_t>::destroy(__a, this);
      allocator_traits<__alloc_t>::deallocate(__a, this, 1);
    }
    // Nothing above may touch any member after this point -- *this is gone.
    if (__was_associated) {
      __token.disassociate();
    }
  }

  enum class __phase : unsigned char { __initial, __consumed, __abandoned, __completed };

  _Alloc __alloc_;
  inplace_stop_source __ssource_;
  connect_result_t<__wrapped_t, __rcvr_t> __op_;
  _Token __token_;
  bool __associated_;
  mutex __mtx_;
  __phase __phase_                                     = __phase::__initial;
  __spawn_future_consume_sink<__sigs_t>* __registered_ = nullptr;
  bool __complete_seen_while_abandoned_                = false;
  bool __request_stop_in_progress_                     = false;
};

// [exec.spawn.future]p14: the outer sender's own opstate. Owns the unique_ptr (so the state is
// abandon()ed -- not directly deleted -- whether this opstate is destroyed before start() is
// ever called, after it, or never at all), and doubles as the consume-sink __complete() dispatches
// through once a registered receiver's result is ready.
template <class _State, class _Rcvr>
class __spawn_future_opstate final : public __spawn_future_consume_sink<typename _State::__sigs_t> {
public:
  using operation_state_concept = operation_state_tag;

  _LIBCPP_HIDE_FROM_ABI __spawn_future_opstate(unique_ptr<_State, __spawn_future_deleter<_State>> __state,
                                               _Rcvr&& __rcvr)
      : __state_(std::move(__state)), __rcvr_(std::move(__rcvr)) {}

  __spawn_future_opstate(const __spawn_future_opstate&)            = delete;
  __spawn_future_opstate& operator=(const __spawn_future_opstate&) = delete;

  _LIBCPP_HIDE_FROM_ABI void start() & noexcept { __state_->__consume(this); }

private:
  _LIBCPP_HIDE_FROM_ABI void __on_complete() noexcept override {
    std::move(__state_->__result)
        .visit(
            [this](auto&& __tup) noexcept {
              if constexpr (!same_as<remove_cvref_t<decltype(__tup)>, monostate>) {
                std::apply(
                    [this](auto __cpo, auto&&... __vals) {
                      __cpo(std::move(__rcvr_), std::forward<decltype(__vals)>(__vals)...);
                    },
                    std::forward<decltype(__tup)>(__tup));
              }
            });
  }

  unique_ptr<_State, __spawn_future_deleter<_State>> __state_;
  _Rcvr __rcvr_;
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
    auto __new_sender     = __token.wrap(std::forward<_Sndr>(__sndr));
    using __new_sender_t  = decltype(__new_sender);
    auto __alloc          = execution::__spawn_select_allocator(__new_sender, __env);
    using __alloc_t       = decltype(__alloc);
    using __state_t       = __spawn_future_state<__alloc_t, _Token, __new_sender_t, _Env>;
    using __rebound_t     = typename allocator_traits<__alloc_t>::template rebind_alloc<__state_t>;

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
  template <class _Self, class _Env>
  _LIBCPP_HIDE_FROM_ABI static consteval auto get_completion_signatures() {
    return __spawn_future_outer_sigs_t<typename _State::__sigs_t>{};
  }
};

} // namespace execution

#endif // _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_THREADS

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP___EXECUTION_ASYNC_SCOPE_H
