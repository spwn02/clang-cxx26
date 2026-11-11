//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___IO_CONTINUATION_H
#define _LIBCPP___IO_CONTINUATION_H

#include <__config>
#include <__coroutine/coroutine_handle.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_EXPERIMENTAL_NETWORKING && _LIBCPP_HAS_THREADS

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

// P4003R3 [4.3]: pairs a coroutine handle with an intrusive link, so that an executor can queue a resumption without
// allocating a node. The object is owned by whoever posts it (typically the awaitable) and must stay alive until the
// executor has resumed (or dropped) it.
//
// __chained_ marks a continuation whose coroutine is owned by something else: a frame in an awaiting chain (owned by
// its parent, up to the root) or a self-owned helper frame that its execution_context destroys on shutdown (see
// execution_context::__frame_node). An executor that is destroyed with such a continuation still queued drops it
// without destroying the coroutine (destroying it would free a frame its owner destroys again, or leave the owner of
// the chain alive); a raw continuation is destroyed.
struct continuation {
  coroutine_handle<> h;
  continuation* next = nullptr;
  bool __chained_    = false;
};

} // namespace io

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_HAS_EXPERIMENTAL_NETWORKING && _LIBCPP_HAS_THREADS

#endif // _LIBCPP___IO_CONTINUATION_H
