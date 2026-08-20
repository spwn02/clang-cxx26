//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___EXECUTION_CONNECT_H
#define _LIBCPP___EXECUTION_CONNECT_H

#include <__config>
#include <__execution/domain.h>
#include <__execution/get_completion_signatures.h>
#include <__execution/get_env.h>
#include <__execution/operation_state.h>
#include <__execution/receiver.h>
#include <__execution/sender.h>
#include <__utility/declval.h>
#include <__utility/forward.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

namespace execution {

template <class _Sndr, class _Rcvr>
_LIBCPP_HIDE_FROM_ABI constexpr auto __connect_impl(_Sndr&& __sndr, _Rcvr&& __rcvr)
    -> decltype(execution::transform_sender(std::forward<_Sndr>(__sndr), execution::get_env(__rcvr))
                    .connect(std::forward<_Rcvr>(__rcvr))) {
  return execution::transform_sender(std::forward<_Sndr>(__sndr), execution::get_env(__rcvr))
      .connect(std::forward<_Rcvr>(__rcvr));
}

// [exec.connect]: only the member-`connect` dispatch (6.1) is implemented. The (6.2)
// fallback, connect-awaitable(new_sndr, rcvr), wraps an awaitable sender in a
// coroutine-backed operation state and needs the same GET-AWAITER/env-promise machinery
// deferred to M6 for enable-sender's second disjunct -- nothing in scope through M5 relies
// on it, since every sender here defines a member `connect`.
struct connect_t {
  template <class _Sndr, class _Rcvr>
    requires sender<_Sndr> && receiver<_Rcvr> && requires(_Sndr&& __sndr, _Rcvr&& __rcvr) {
      { execution::__connect_impl(std::forward<_Sndr>(__sndr), std::forward<_Rcvr>(__rcvr)) } -> operation_state;
    }
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Sndr&& __sndr, _Rcvr&& __rcvr) const
      noexcept(noexcept(execution::__connect_impl(std::forward<_Sndr>(__sndr), std::forward<_Rcvr>(__rcvr)))) {
    static_assert(sender_in<_Sndr, env_of_t<_Rcvr>>, "Mandates: sender_in<Sndr, env_of_t<Rcvr>>.");
    static_assert(receiver_of<_Rcvr, completion_signatures_of_t<_Sndr, env_of_t<_Rcvr>>>,
                  "Mandates: receiver_of<Rcvr, completion_signatures_of_t<Sndr, env_of_t<Rcvr>>>.");
    return execution::__connect_impl(std::forward<_Sndr>(__sndr), std::forward<_Rcvr>(__rcvr));
  }
};

inline constexpr connect_t connect{};

template <class _Sndr, class _Rcvr>
using connect_result_t = decltype(execution::connect(std::declval<_Sndr>(), std::declval<_Rcvr>()));

// [exec.snd.concepts]: sender-to.
template <class _Sndr, class _Rcvr>
concept __sender_to = sender_in<_Sndr, env_of_t<_Rcvr>> && receiver_of<_Rcvr, completion_signatures_of_t<_Sndr, env_of_t<_Rcvr>>> &&
                       requires(_Sndr&& __sndr, _Rcvr&& __rcvr) {
                         execution::connect(std::forward<_Sndr>(__sndr), std::forward<_Rcvr>(__rcvr));
                       };

} // namespace execution

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___EXECUTION_CONNECT_H
