//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___NET_FAMILY_H
#define _LIBCPP___NET_FAMILY_H

#include <__config>
#include <__io/support/config.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_IO_SOCKETS

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

// P4100R1 [8.9]; provisional specification: Corosio's family and shutdown_type. The address family of an IP socket.
// Sockets that are not IP sockets (Unix domain) pass v4 to their options.
enum class family { v4, v6 };

// The directions of shutdown(); the values are the POSIX SHUT_RD, SHUT_WR and SHUT_RDWR.
enum shutdown_type { shutdown_receive, shutdown_send, shutdown_both };

} // namespace io

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_HAS_IO_SOCKETS

#endif // _LIBCPP___NET_FAMILY_H
