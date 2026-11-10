//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___IO_IO_ENV_H
#define _LIBCPP___IO_IO_ENV_H

#include <__config>
#include <__io/executor_ref.h>
#include <__memory_resource/memory_resource.h>
#include <__stop_token/stop_token.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_EXPERIMENTAL_NETWORKING && _LIBCPP_HAS_THREADS

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

// P4003R3 [4.1]: what a coroutine chain needs at every suspension point. The launch function owns the io_env and every
// coroutine in the chain borrows it through an `io_env const*`.
struct io_env {
  executor_ref executor;
  std::stop_token stop_token;
  pmr::memory_resource* frame_allocator = nullptr;
};

} // namespace io

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_HAS_EXPERIMENTAL_NETWORKING && _LIBCPP_HAS_THREADS

#endif // _LIBCPP___IO_IO_ENV_H
