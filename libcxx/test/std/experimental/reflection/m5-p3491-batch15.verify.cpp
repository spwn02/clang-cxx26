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
// ADDITIONAL_COMPILE_FLAGS: -Xclang -verify-ignore-unexpected=note

#include <array>
#include <meta>

namespace p3491_batch15 {

struct NonStructural {
private:
  [[maybe_unused]] int value;
public:
  constexpr NonStructural(int v) : value(v) {}
};

constexpr auto bad_structural = std::meta::reflect_constant_array(
    std::array{NonStructural{1}});
// FIXME(spwn02/clang-cxx26#127): P3491R3 makes this a Mandates, i.e. a hard
// error at the call, not a removed overload.
// expected-error@-4 {{no matching function for call to 'reflect_constant_array'}}

constexpr auto values = std::define_static_array(std::array{2, 4, 6});
static_assert(values.size() == 3);
static_assert(values[0] == 2 && values[2] == 6);

} // namespace p3491_batch15
