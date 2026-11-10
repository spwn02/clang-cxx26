//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___IO_IO_AWAITABLE_H
#define _LIBCPP___IO_IO_AWAITABLE_H

#include <__concepts/same_as.h>
#include <__config>
#include <__coroutine/coroutine_handle.h>
#include <__exception/exception_ptr.h>
#include <__io/io_env.h>
#include <__type_traits/is_void.h>
#include <__utility/declval.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_EXPERIMENTAL_NETWORKING && _LIBCPP_HAS_THREADS

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

// P4003R3 [4.2]: the two-argument await_suspend is the protocol boundary. The caller's await_transform injects the
// environment, so an awaitable from another coroutine model fails to compile instead of misbehaving at run time.
template <class _Ap>
concept IoAwaitable = requires(_Ap __a, coroutine_handle<> __h, const io_env* __env) { __a.await_suspend(__h, __env); };

// P4003R3 [4.6]: what a launch function needs from a task type (the paper proposes the concept, not the launch
// functions).
template <class _Tp>
concept IoRunnable =
    IoAwaitable<_Tp> && requires { typename _Tp::promise_type; } &&
    requires(_Tp& __t, const _Tp& __ct, const typename _Tp::promise_type& __cp, typename _Tp::promise_type& __p) {
      { __ct.handle() } noexcept -> same_as<coroutine_handle<typename _Tp::promise_type>>;
      { __cp.exception() } noexcept -> same_as<exception_ptr>;
      { __t.release() } noexcept;
      { __p.set_continuation(coroutine_handle<>{}) } noexcept;
      { __p.set_environment(static_cast<const io_env*>(nullptr)) } noexcept;
    } && (is_void_v<decltype(std::declval<_Tp&>().await_resume())> || requires(typename _Tp::promise_type& __p) {
      __p.result();
    });

} // namespace io

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_HAS_EXPERIMENTAL_NETWORKING && _LIBCPP_HAS_THREADS

#endif // _LIBCPP___IO_IO_AWAITABLE_H
