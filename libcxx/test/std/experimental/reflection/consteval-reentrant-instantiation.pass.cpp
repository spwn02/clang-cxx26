//===----------------------------------------------------------------------===//
//
// Copyright 2024 Bloomberg Finance L.P.
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

// <experimental/reflection>
//
// [reflection]

#include <meta>
#include <cassert>
#include <vector>

template <int N>
consteval auto chain() {
  if constexpr (N <= 0) {
    return 0;
  } else {
    return [:std::meta::substitute(
        ^^chain,
        std::vector<std::meta::info>{std::meta::reflect_constant(N - 1)}):]() + 1;
  }
}

consteval int start(int n) {
  return std::meta::extract<int (*)()>(std::meta::substitute(
      ^^chain, std::vector<std::meta::info>{std::meta::reflect_constant(n)}))();
}

int main() {
  int x = start(64);
  assert(x == 64);
  return 0;
}
