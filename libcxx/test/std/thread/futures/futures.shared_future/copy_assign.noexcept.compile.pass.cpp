//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03
// UNSUPPORTED: no-threads

// <future>

// shared_future& operator=(const shared_future& rhs) noexcept; (also for shared_future<R&> and shared_future<void>)

#include <future>
#include <type_traits>

static_assert(std::is_nothrow_copy_assignable_v<std::shared_future<int>>);
static_assert(std::is_nothrow_copy_assignable_v<std::shared_future<int&>>);
static_assert(std::is_nothrow_copy_assignable_v<std::shared_future<void>>);
