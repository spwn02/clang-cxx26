//===----------------------------------------------------------------------===//
//
// Copyright 2026
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest
// expected-no-diagnostics

#include <meta>

struct S {
  S();
  ~S();
};
struct T { constexpr T(int) {} };

constexpr auto constructor = ^^S::S;
constexpr auto destructor = ^^S::~S;

template <std::meta::info R>
consteval auto dependent_ctad() {
  [:R:] value = {1};
  return value;
}

using Dependent = decltype(dependent_ctad<^^T>());
