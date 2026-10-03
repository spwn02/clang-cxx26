//===----------------------------------------------------------------------===//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20 || c++23
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest -fattribute-reflection

#include <meta>

// P3385R8 [meta.reflection.define.aggregate] declares the attributes member before
// annotations in data_member_options.
static_assert(std::meta::identifier_of(std::meta::nonstatic_data_members_of(
                  ^^std::meta::data_member_options, std::meta::access_context::current())[4]) == "attributes");
static_assert(std::meta::identifier_of(std::meta::nonstatic_data_members_of(
                  ^^std::meta::data_member_options, std::meta::access_context::current())[5]) == "annotations");

// P3385R8 would deprecate no_unique_address, but it is an unadopted C++29-target
// proposal: the C++26 member is not deprecated, so using it must not warn (the test
// harness compiles with -Werror).
consteval bool uses_no_unique_address() {
  (void)std::meta::data_member_spec(^^int, {.name = "m", .no_unique_address = true});
  return true;
}
static_assert(uses_no_unique_address());

int main(int, char**) {}
