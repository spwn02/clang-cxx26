//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

#include <meta>
#include <vector>

void runtime_push_back() {
  std::vector<std::meta::info> v;
  v.push_back(^^int); // expected-error {{consteval-only value is only allowed}}
}
