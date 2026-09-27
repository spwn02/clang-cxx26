// -*- C++ -*-
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// ADDITIONAL_COMPILE_FLAGS: -ffreestanding

// P1642R11 [functional.syn]: selected functional facilities are freestanding.

#include <functional>
#include <version>

#ifndef _LIBCPP_FREESTANDING
#  error "-ffreestanding must select libc++ freestanding mode"
#endif
#if !defined(__cpp_lib_freestanding_functional) || __cpp_lib_freestanding_functional != 202306L
#  error "missing or wrong __cpp_lib_freestanding_functional"
#endif

struct Callable {
  constexpr int operator()(int first, int second) const { return first + second; }
};

void test_functional() {
  int value = 1;
  auto reference = std::ref(value);
  auto bound = std::bind(Callable{}, std::placeholders::_1, 2);
  auto front = std::bind_front(Callable{}, 3);
  auto back = std::bind_back(Callable{}, 4);
  int invoked = std::invoke(Callable{}, 1, 2);
  int invoked_r = std::invoke_r<int>(Callable{}, 1, 2);
  bool comparison = std::less<>{}(1, 2) && std::equal_to<>{}(1, 1);
  int arithmetic = std::plus<>{}(1, 2) + std::minus<>{}(4, 1) + std::multiplies<>{}(2, 3);
  auto negated = std::not_fn([] { return false; });
  auto member = std::mem_fn(&Callable::operator());
  auto hash = std::hash<int>{}(value);
  auto identity = std::identity{}(value);
  (void)reference; (void)bound; (void)front; (void)back; (void)invoked; (void)invoked_r;
  (void)comparison; (void)arithmetic; (void)negated; (void)member; (void)hash; (void)identity;
}
