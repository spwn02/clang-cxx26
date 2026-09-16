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
#include <__execution/get_env.h>
#include <__execution/operation_state.h>
#include <__execution/receiver.h>
#include <__execution/sender.h>
#include <__memory/allocator.h>
#include <__memory/allocator_traits.h>
#include <__mutex/lock_guard.h>
#include <__mutex/mutex.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/forward.h>
#include <__utility/move.h>
#include <cstddef>
#include <exception>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_PUSH_MACROS
#include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

// execution::async_scope (P3149R11). See docs/design/async_scope_p3149.md for the design
// note this implementation follows, including the staging plan: this file implements Pass 1
// only (scope_token, simple_counting_scope, associate, spawn, join) -- counting_scope's
// stop-forwarding token::wrap() (Pass 2) and spawn_future (Pass 3) are explicitly deferred,
// tracked as follow-up work, not silently dropped.
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

} // namespace execution

#endif // _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_THREADS

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP___EXECUTION_ASYNC_SCOPE_H
