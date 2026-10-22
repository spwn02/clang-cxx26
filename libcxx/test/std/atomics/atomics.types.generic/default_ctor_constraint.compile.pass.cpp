//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17

// <atomic>

// LWG4169: atomic<T>::atomic() is constrained on is_default_constructible_v<T>.

#include <atomic>
#include <type_traits>

struct NoDefault {
  NoDefault(int);
};

struct Default {
  Default() = default;
};

static_assert(!std::is_default_constructible_v<std::atomic<NoDefault>>);
static_assert(std::is_default_constructible_v<std::atomic<Default>>);
static_assert(std::is_default_constructible_v<std::atomic<int>>);
