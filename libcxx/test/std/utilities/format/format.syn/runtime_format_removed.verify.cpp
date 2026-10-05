//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <format>

// P3953R3 renamed std::runtime_format to std::dynamic_format; the old name is not part of the draft.

#include <format>
#include <string_view>

void test() {
  (void)std::dynamic_format(std::string_view("{}"));
  (void)std::runtime_format(std::string_view("{}")); // expected-error {{no member named 'runtime_format' in namespace 'std'}}
}
