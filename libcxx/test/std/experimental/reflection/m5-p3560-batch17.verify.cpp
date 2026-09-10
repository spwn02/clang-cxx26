//===----------------------------------------------------------------------===//
//
// Copyright 2026 Bloomberg Finance L.P.
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

namespace batch17 {

consteval bool test_inaccessible_member_query() {
  try {
    std::meta::has_inaccessible_nonstatic_data_members(
        ^^int, std::meta::access_context::current());
    return false;
  } catch (const std::meta::exception& e) {
    return e.from() == ^^std::meta::has_inaccessible_nonstatic_data_members;
  }
}
static_assert(test_inaccessible_member_query());

consteval bool test_inaccessible_base_query() {
  try {
    std::meta::has_inaccessible_bases(^^int,
                                     std::meta::access_context::current());
    return false;
  } catch (const std::meta::exception& e) {
    return e.from() == ^^std::meta::has_inaccessible_bases;
  }
}
static_assert(test_inaccessible_base_query());

consteval bool test_layout_queries() {
  try {
    std::meta::size_of(^^void);
    return false;
  } catch (const std::meta::exception& e) {
    if (e.from() != ^^std::meta::size_of)
      return false;
  }
  try {
    std::meta::bit_size_of(^^void);
    return false;
  } catch (const std::meta::exception& e) {
    if (e.from() != ^^std::meta::bit_size_of)
      return false;
  }
  try {
    std::meta::alignment_of(^^void);
    return false;
  } catch (const std::meta::exception& e) {
    return e.from() == ^^std::meta::alignment_of;
  }
}
static_assert(test_layout_queries());

consteval bool test_template_queries() {
  try {
    std::meta::template_of(^^int);
    return false;
  } catch (const std::meta::exception& e) {
    if (e.from() != ^^std::meta::template_of)
      return false;
  }
  return true;
}
static_assert(test_template_queries());

consteval bool test_domain_queries() {
  try {
    std::meta::operator_of(^^int);
    return false;
  } catch (const std::meta::exception& e) {
    if (e.from() != ^^std::meta::operator_of)
      return false;
  }
  return true;
}
static_assert(test_domain_queries());

} // namespace batch17
