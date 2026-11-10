//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___IO_FRAME_ALLOCATOR_H
#define _LIBCPP___IO_FRAME_ALLOCATOR_H

#include <__config>
#include <__coroutine/coroutine_handle.h>
#include <__memory_resource/memory_resource.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_EXPERIMENTAL_NETWORKING && _LIBCPP_HAS_THREADS

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

// P4003R3 [4.5] / P4172R1 [8.2]: the out-of-band channel that delivers the frame allocator to the promise's
// operator new. The paper leaves the storage to the implementation. This one is a thread-local pointer, which is the
// default for hosted, multi-threaded environments; without thread support it would have to be a plain global, which is
// only correct when a single thread runs coroutine chains. The slot is per program image (each shared object that
// includes this header has its own).
_LIBCPP_HIDE_FROM_ABI inline pmr::memory_resource*& __cached_frame_allocator_slot() noexcept {
#  if _LIBCPP_HAS_THREADS
  static thread_local pmr::memory_resource* __slot = nullptr;
#  else
  static pmr::memory_resource* __slot = nullptr;
#  endif
  return __slot;
}

_LIBCPP_HIDE_FROM_ABI inline pmr::memory_resource* get_cached_frame_allocator() noexcept {
  return __cached_frame_allocator_slot();
}

_LIBCPP_HIDE_FROM_ABI inline void set_cached_frame_allocator(pmr::memory_resource* __mr) noexcept {
  __cached_frame_allocator_slot() = __mr;
}

// P4172R1 [8.3]: every loop that resumes a coroutine handle saves the slot before the resumption and restores it
// afterwards, so that a resumed chain cannot spoil the allocator that the resuming code was about to hand to a child
// frame. The slot therefore behaves like a stack.
_LIBCPP_HIDE_FROM_ABI inline void safe_resume(coroutine_handle<> __h) noexcept {
  pmr::memory_resource* __saved = get_cached_frame_allocator();
  __h.resume();
  set_cached_frame_allocator(__saved);
}

} // namespace io

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_HAS_EXPERIMENTAL_NETWORKING && _LIBCPP_HAS_THREADS

#endif // _LIBCPP___IO_FRAME_ALLOCATOR_H
