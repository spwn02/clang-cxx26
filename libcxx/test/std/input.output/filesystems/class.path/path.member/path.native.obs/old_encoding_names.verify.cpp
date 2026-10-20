//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

#include <filesystem>

void test(const std::filesystem::path& p) {
  p.system_encoded_string(); // expected-error {{no member named 'system_encoded_string' in 'std::filesystem::path'}}
  p.generic_system_encoded_string(); // expected-error {{no member named 'generic_system_encoded_string' in 'std::filesystem::path'}}
}
