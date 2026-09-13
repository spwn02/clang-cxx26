//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

#include <meta>
#include <string_view>

struct complete { int value; };

consteval bool catches_members_error() {
  try {
    (void)std::meta::members_of(
        ^^int, std::meta::access_context::unchecked());
    return false;
  } catch (const std::meta::exception& e) {
    return e.from() != std::meta::info{} &&
           e.u8what() == u8"invalid reflection operand" &&
           std::string_view(e.what()) == "invalid reflection operand" &&
           e.where().line() != 0;
  }
}

consteval bool inner() { return catches_members_error(); }
consteval bool outer() { return inner(); }

consteval bool catches_representative_failures() {
  bool type = false;
  bool parent = false;
  bool bases = false;
  try { (void)std::meta::type_of(^^std); }
  catch (const std::meta::exception& e) {
    type = e.from() == ^^std::meta::type_of;
  }
  try { (void)std::meta::parent_of(^^int); }
  catch (const std::meta::exception& e) {
    parent = e.from() == ^^std::meta::parent_of;
  }
  try {
    (void)std::meta::bases_of(
        ^^int, std::meta::access_context::unchecked());
  } catch (const std::meta::exception& e) {
    bases = e.from() != std::meta::info{};
  }
  return type && parent && bases;
}

static_assert(catches_members_error());
static_assert(outer());
static_assert(catches_representative_failures());
static_assert(std::meta::members_of(
                  ^^complete, std::meta::access_context::unchecked()).size() >=
              1);

int main(int, char**) { return 0; }
