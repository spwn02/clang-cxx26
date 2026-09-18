//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___EXECUTION_STOP_WHEN_H
#define _LIBCPP___EXECUTION_STOP_WHEN_H

#include <__concepts/invocable.h>
#include <__config>
#include <__execution/completion_signatures.h>
#include <__execution/connect.h>
#include <__execution/env.h>
#include <__execution/fwd_env.h>
#include <__execution/get_completion_signatures.h>
#include <__execution/get_env.h>
#include <__execution/get_stop_token.h>
#include <__execution/operation_state.h>
#include <__execution/receiver.h>
#include <__execution/sender.h>
#include <__execution/write_env.h>
#include <__stop_token/inplace_stop_source.h>
#include <__stop_token/inplace_stop_token.h>
#include <__stop_token/stoppable_token.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/declval.h>
#include <__utility/forward.h>
#include <__utility/move.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_PUSH_MACROS
#include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_THREADS

namespace execution {

// [exec.stop.when] (exposition-only): P3149R11 adds this immediately after [exec.associate]
// specifically to define counting_scope::token::wrap() and spawn_future() in terms of it. It is
// never a name in [execution.syn] at all -- unlike write_env (a real, if unspecified-type,
// customization point object, see <__execution/write_env.h>'s own __write_env_t) -- so both the
// type AND the instance below are __-prefixed and never exported (see
// libcxx/modules/std/execution.inc): nothing outside this fork's own async_scope machinery
// should ever name __stop_when directly.
//
// stop-when(sndr, token) fuses an additional stop token into a sender: the operation state
// produced by connecting the result receives a stop request when EITHER `token` or the
// connected receiver's own get_stop_token(get_env(r)) requests one. [exec.stop.when]p3's first
// branch (`token` itself models unstoppable_token => stop-when(sndr, token) is
// expression-equivalent to sndr) changes stop-when's own static type, so it lives in
// __stop_when_t::operator() below, not inside connect(). The remaining two branches (p3.1/p3.2)
// are both implemented by installing an appropriate stop token into the child's environment via
// write_env -- reusing that adaptor's already-correct join-env/get_completion_signatures
// machinery rather than reimplementing it (see __stop_when_opstate below).
template <class _Token, class _Sndr>
class __stop_when_sndr;

struct __stop_when_t {
  template <sender _Sndr, stoppable_token _Token>
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Sndr&& __sndr, _Token __token) const {
    if constexpr (unstoppable_token<_Token>) {
      return std::forward<_Sndr>(__sndr);
    } else {
      return __stop_when_sndr<_Token, remove_cvref_t<_Sndr>>{{}, std::move(__token), std::forward<_Sndr>(__sndr)};
    }
  }
};

inline constexpr __stop_when_t __stop_when{};

// The callback functor both stop-forwarding callbacks in the p3.2 (combined) case share:
// touches only __source (never the enclosing __stop_when_opstate or its child), which is what
// makes it safe for either callback to fire synchronously during that opstate's own
// construction (registering a callback on a token whose stop was already requested invokes it
// immediately) -- __source is always already fully constructed by the time either callback is
// constructed, see __stop_when_opstate<true, ...>'s member declaration order below.
struct __stop_when_forwarder {
  inplace_stop_source* __source;
  _LIBCPP_HIDE_FROM_ABI void operator()() noexcept { __source->request_stop(); }
};

template <bool _Combined, class _Token, class _Sndr, class _Rcvr>
class __stop_when_opstate;

// [exec.stop.when]p3.1: the connected receiver's own stop token models unstoppable_token --
// connect(write_env(sndr, prop(get_stop_token, token)), r) directly, no combined source needed.
template <class _Token, class _Sndr, class _Rcvr>
class __stop_when_opstate<false, _Token, _Sndr, _Rcvr> {
  using __env_t      = decltype(execution::prop(std::get_stop_token, std::declval<_Token>()));
  using __wrapped_t  = decltype(execution::write_env(std::declval<_Sndr>(), std::declval<__env_t>()));
  using __child_op_t = connect_result_t<__wrapped_t, _Rcvr>;

public:
  using operation_state_concept = operation_state_tag;

  _LIBCPP_HIDE_FROM_ABI __stop_when_opstate(_Token&& __token, _Sndr&& __sndr, _Rcvr&& __rcvr)
      : __child_(execution::connect(
            execution::write_env(std::forward<_Sndr>(__sndr),
                                  execution::prop(std::get_stop_token, std::forward<_Token>(__token))),
            std::forward<_Rcvr>(__rcvr))) {}

  __stop_when_opstate(const __stop_when_opstate&)            = delete;
  __stop_when_opstate& operator=(const __stop_when_opstate&) = delete;

  _LIBCPP_HIDE_FROM_ABI void start() & noexcept { execution::start(__child_); }

private:
  __child_op_t __child_;
};

// [exec.stop.when]p3.2: neither `token` nor the receiver's own stop token models
// unstoppable_token -- build a combined inplace_stop_source that either side can request stop
// on, forward both `token` and the receiver's own stop token into it via one callback each, and
// install the combined source's own token as the child's stop token. This is a concrete
// realization of the paper's abstract exposition-only `stoken-t` (whose stop_requested()/
// stop_possible() are specified as the OR of both inputs): an inplace_stop_token backed by a
// source that either input can independently request stop on models that OR relationship
// directly, without needing a bespoke combinator token type.
template <class _Token, class _Sndr, class _Rcvr>
class __stop_when_opstate<true, _Token, _Sndr, _Rcvr> {
  using __rtoken_t   = stop_token_of_t<decltype(execution::get_env(std::declval<_Rcvr&>()))>;
  using __env_t      = decltype(execution::prop(std::get_stop_token, std::declval<inplace_stop_token>()));
  using __wrapped_t  = decltype(execution::write_env(std::declval<_Sndr>(), std::declval<__env_t>()));
  using __child_op_t = connect_result_t<__wrapped_t, _Rcvr>;

public:
  using operation_state_concept = operation_state_tag;

  _LIBCPP_HIDE_FROM_ABI __stop_when_opstate(_Token&& __token, _Sndr&& __sndr, _Rcvr&& __rcvr)
      : __rtoken_(std::get_stop_token(execution::get_env(__rcvr))),
        __token_cb_(std::forward<_Token>(__token), __stop_when_forwarder{&__source_}),
        __rtoken_cb_(__rtoken_, __stop_when_forwarder{&__source_}),
        __child_(execution::connect(
            execution::write_env(std::forward<_Sndr>(__sndr),
                                  execution::prop(std::get_stop_token, __source_.get_token())),
            std::forward<_Rcvr>(__rcvr))) {}

  __stop_when_opstate(const __stop_when_opstate&)            = delete;
  __stop_when_opstate& operator=(const __stop_when_opstate&) = delete;

  _LIBCPP_HIDE_FROM_ABI void start() & noexcept { execution::start(__child_); }

private:
  // Declaration order is load-bearing (members are constructed top-to-bottom, destroyed
  // bottom-to-top regardless of initializer-list order): __source_ first, so it's already valid
  // if either callback fires synchronously during its own construction; the two callbacks next,
  // so they're unregistered (destroyed) before __source_ can be destroyed; __child_ last, so the
  // wrapped operation is fully torn down before the stop-forwarding machinery that fed it.
  inplace_stop_source __source_;
  __rtoken_t __rtoken_;
  stop_callback_for_t<_Token, __stop_when_forwarder> __token_cb_;
  stop_callback_for_t<__rtoken_t, __stop_when_forwarder> __rtoken_cb_;
  __child_op_t __child_;
};

// An aggregate with public `tag`/`data`/`child` members, matching the (tag, data, ...children)
// shape tag_of_t (<__execution/sender.h>) decomposes via structured bindings -- not routed
// through the draft's generic basic-sender/impls-for machinery: see the M3 entry in
// docs/CXX26_GAPS.md for why that engine isn't buildable on this fork yet.
template <class _Token, class _Sndr>
class __stop_when_sndr {
public:
  using sender_concept = sender_tag;

  _LIBCPP_NO_UNIQUE_ADDRESS __stop_when_t tag;
  _Token data;
  _Sndr child;

  template <class _Rcvr>
  _LIBCPP_HIDE_FROM_ABI constexpr auto connect(_Rcvr&& __rcvr) && {
    using __rtoken_t                = stop_token_of_t<decltype(execution::get_env(__rcvr))>;
    constexpr bool __combined       = !unstoppable_token<__rtoken_t>;
    return __stop_when_opstate<__combined, _Token, _Sndr, remove_cvref_t<_Rcvr>>(
        std::move(data), std::move(child), std::forward<_Rcvr>(__rcvr));
  }

  // [exec.adapt.general]p3.2: a parent sender with a single child sndr has an associated
  // attribute object equal to FWD-ENV(get_env(sndr)) -- stop-when doesn't customize its own
  // attributes (only the environment its child is connected through, mirroring write_env).
  _LIBCPP_HIDE_FROM_ABI constexpr auto get_env() const noexcept {
    return execution::__fwd_env_fn(execution::get_env(child));
  }

  // Delegates to write_env's own already-correct join-env computation for whichever branch
  // connect() will actually take, rather than reimplementing it: the joined environment (and
  // therefore the completion signatures) is identical either way to what
  // write_env(child, prop(get_stop_token, <token>)) would compute for the same _Env.
  template <class _Self, class _Env>
  _LIBCPP_HIDE_FROM_ABI static consteval auto get_completion_signatures() {
    using __rtoken_t = stop_token_of_t<_Env>;
    if constexpr (unstoppable_token<__rtoken_t>) {
      using __env_t     = decltype(execution::prop(std::get_stop_token, std::declval<_Token>()));
      using __wrapped_t = decltype(execution::write_env(std::declval<_Sndr>(), std::declval<__env_t>()));
      return completion_signatures_of_t<__wrapped_t, _Env>{};
    } else {
      using __env_t     = decltype(execution::prop(std::get_stop_token, std::declval<inplace_stop_token>()));
      using __wrapped_t = decltype(execution::write_env(std::declval<_Sndr>(), std::declval<__env_t>()));
      return completion_signatures_of_t<__wrapped_t, _Env>{};
    }
  }
};

} // namespace execution

#endif // _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_THREADS

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP___EXECUTION_STOP_WHEN_H
