//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___IO_SUPPORT_POLLER_TYPES_H
#define _LIBCPP___IO_SUPPORT_POLLER_TYPES_H

#include <__config>
#include <cstdint>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

// The platform-neutral readiness bits. __poll_error is reported for error and hang-up conditions; it is never
// requested, a backend always reports it.
inline constexpr unsigned __poll_read  = 1;
inline constexpr unsigned __poll_write = 2;
inline constexpr unsigned __poll_error = 4;

struct __poller_event {
  uintptr_t __tag_;
  unsigned __events_;
};

} // namespace io

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___IO_SUPPORT_POLLER_TYPES_H
