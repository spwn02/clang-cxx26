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

#include <meta>

template <class T>
struct S;

consteval {
  std::meta::define_aggregate(^^S<int>, {
    std::meta::data_member_spec(^^int, {.name = "x"}),
  });
}

static_assert(std::meta::is_complete_type(^^S<int>));
static_assert(std::meta::nonstatic_data_members_of(^^S<int>, std::meta::access_context::unchecked()).size() == 1);

int main(int, char**) {
  S<int> value{42};
  return value.x == 42 ? 0 : 1;
}
