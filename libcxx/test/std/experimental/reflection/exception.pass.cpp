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
// ADDITIONAL_COMPILE_FLAGS: -freflection -fparameter-reflection

// <meta>

#include <cassert>
#include <climits>
#include <meta>
#include <string_view>

using std::meta::exception;
using std::meta::info;

consteval bool test_u8_constructor() {
  exception e(u8"error", ^^int);
  return e.u8what() == u8"error" &&
         e.what()[0] == 'e' && e.from() == (^^int) &&
         e.where().line() != 0;
}

consteval bool test_string_constructor() {
  exception e("error", ^^int);
  return e.u8what() == u8"error" &&
         std::string_view(e.what()) == "error" && e.from() == (^^int);
}

consteval bool test_throw_and_catch() {
  try {
    throw exception(u8"caught", ^^int);
  } catch (const exception& e) {
    return e.u8what() == u8"caught" && e.from() == (^^int);
  }
  return false;
}

consteval bool test_layout_query_preconditions() {
  return std::meta::size_of(^^int) == sizeof(int) &&
         std::meta::bit_size_of(^^int) == sizeof(int) * CHAR_BIT &&
         std::meta::alignment_of(^^int) == alignof(int);
}

struct member_query_record {
  static constexpr int value = 0;
  int member;
};

consteval bool test_member_query_preconditions() {
  constexpr auto ctx = std::meta::access_context::current();
  return std::meta::members_of(^^member_query_record, ctx).size() >= 1 &&
         std::meta::bases_of(^^member_query_record, ctx).empty() &&
         std::meta::static_data_members_of(^^member_query_record, ctx).size() >= 1 &&
         std::meta::nonstatic_data_members_of(^^member_query_record, ctx).size() >= 1;
}

static_assert(test_u8_constructor());
static_assert(test_string_constructor());
static_assert(test_throw_and_catch());
static_assert(test_layout_query_preconditions());
static_assert(test_member_query_preconditions());

int main() {
  assert(test_u8_constructor());
  assert(test_string_constructor());
  assert(test_throw_and_catch());
  assert(test_layout_query_preconditions());
  assert(test_member_query_preconditions());
}
