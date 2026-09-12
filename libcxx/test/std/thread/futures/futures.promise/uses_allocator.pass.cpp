//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// UNSUPPORTED: no-threads

// <future>

// class promise<R>

// template <class R, class Alloc>
//   struct uses_allocator<promise<R>, Alloc>
//      : true_type { };
//
// Removed for C++26 by P3503R3.

#include <future>
#include "test_macros.h"
#include "test_allocator.h"

int main(int, char**)
{
#if TEST_STD_VER >= 26
    static_assert(!std::uses_allocator<std::promise<int>, test_allocator<int> >::value, "");
    static_assert(!std::uses_allocator<std::promise<int&>, test_allocator<int> >::value, "");
    static_assert(!std::uses_allocator<std::promise<void>, test_allocator<void> >::value, "");
#else
    static_assert((std::uses_allocator<std::promise<int>, test_allocator<int> >::value), "");
    static_assert((std::uses_allocator<std::promise<int&>, test_allocator<int> >::value), "");
    static_assert((std::uses_allocator<std::promise<void>, test_allocator<void> >::value), "");
#endif

  return 0;
}
