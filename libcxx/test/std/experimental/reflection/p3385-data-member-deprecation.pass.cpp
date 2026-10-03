//===----------------------------------------------------------------------===//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20 || c++23
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest -fattribute-reflection -Wno-deprecated-declarations

#include <meta>

// P3385R8 [meta.reflection.define.aggregate]: deprecate no_unique_address
// in favor of the attributes member of data_member_options.
static_assert(std::meta::has_attribute(^^std::meta::data_member_options::no_unique_address,
                                      ^^[[deprecated]],
                                      std::meta::attribute_comparison::ignore_argument));
// P3385R8 data_member_options synopsis declares attributes before annotations.
static_assert(std::meta::identifier_of(std::meta::nonstatic_data_members_of(
                  ^^std::meta::data_member_options, std::meta::access_context::current())[4]) == "attributes");
static_assert(std::meta::identifier_of(std::meta::nonstatic_data_members_of(
                  ^^std::meta::data_member_options, std::meta::access_context::current())[5]) == "annotations");
int main(int, char**) {}
