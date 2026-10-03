//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20 || c++23
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

#include <meta>

// [meta.reflection.constant]: constant_of(r) is equivalent to
// reflect_constant([: R :]) for a non-array, non-function, non-annotation.
struct K { const char* p; };
constexpr K invalid{"ebab"};
constexpr char valid_string[] = "valid";
constexpr K valid{valid_string};

consteval bool rejects(std::meta::info r) {
  try {
    (void)std::meta::constant_of(r);
  } catch (std::meta::exception& e) {
    return e.from() == ^^std::meta::constant_of;
  }
  return false;
}

static_assert(rejects(^^invalid));
static_assert(rejects(std::meta::reflect_object(invalid)));
static_assert(std::meta::constant_of(^^valid) == std::meta::reflect_constant(valid));
static_assert(std::meta::constant_of(std::meta::reflect_object(valid)) == std::meta::reflect_constant(valid));

int main(int, char**) { return 0; }
