//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef _LIBCPP___NET_MESSAGE_FLAGS_H
#define _LIBCPP___NET_MESSAGE_FLAGS_H

#include <__config>
#include <__io/support/config.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_IO_SOCKETS

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

// P4100R1 [8.9]; provisional specification: Corosio's message_flags. Platform-independent flags of the datagram send and
// receive operations; the platform layer maps them to MSG_PEEK, MSG_OOB and MSG_DONTROUTE. The values (1, 2, 4) are
// those of the platform layer's translation and are not otherwise meaningful.
enum class message_flags : int { none = 0, peek = 1, out_of_band = 2, do_not_route = 4 };

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr message_flags operator|(message_flags __a, message_flags __b) noexcept {
  return static_cast<message_flags>(static_cast<int>(__a) | static_cast<int>(__b));
}
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr message_flags operator&(message_flags __a, message_flags __b) noexcept {
  return static_cast<message_flags>(static_cast<int>(__a) & static_cast<int>(__b));
}
// The complement within the defined flags.
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr message_flags operator~(message_flags __a) noexcept {
  return static_cast<message_flags>(~static_cast<int>(__a) & 7);
}

} // namespace io

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_HAS_IO_SOCKETS

#endif // _LIBCPP___NET_MESSAGE_FLAGS_H
