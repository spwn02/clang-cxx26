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

#include <experimental/meta>
#include <string_view>

namespace single {
struct Plain { int f() const; };
int Plain::f() const { return 1; }
template <class T> struct Tmpl { int g() const; };
template <class T> int Tmpl<T>::g() const { return 2; }
inline int h() { return 3; }
}  // namespace single

namespace re {
template <class T> struct Tmpl { int g() const; };
inline int h() { return 3; }
}  // namespace re

namespace re {
template <class T> int Tmpl<T>::g() const { return 2; }
inline int k() { return 4; }
struct Plain { int f() const; };
}  // namespace re

namespace re {
int Plain::f() const { return 1; }
inline int m() { return 5; }
}  // namespace re

consteval bool ns_has_member(std::meta::info ns, std::string_view name) {
  for (auto m : std::meta::members_of(
           ns, std::meta::access_context::unchecked()))
    if (std::meta::has_identifier(m) && std::meta::identifier_of(m) == name)
      return true;
  return false;
}

consteval std::size_t ns_member_count(std::meta::info ns) {
  std::size_t n = 0;
  for (auto m : std::meta::members_of(
           ns, std::meta::access_context::unchecked())) {
    (void)m;
    ++n;
  }
  return n;
}

int main() {
  static_assert(ns_has_member(^^single, "Plain"));
  static_assert(ns_has_member(^^single, "Tmpl"));
  static_assert(ns_has_member(^^single, "h"));
  static_assert(!ns_has_member(^^single, "f"));
  static_assert(!ns_has_member(^^single, "g"));
  static_assert(ns_member_count(^^single) == 3);

  static_assert(ns_has_member(^^re, "Tmpl"));
  static_assert(ns_has_member(^^re, "Plain"));
  static_assert(ns_has_member(^^re, "h"));
  static_assert(ns_has_member(^^re, "k"));
  static_assert(ns_has_member(^^re, "m"));
  static_assert(!ns_has_member(^^re, "g"));
  static_assert(!ns_has_member(^^re, "f"));
  static_assert(ns_member_count(^^re) == 5);

  consteval {
    std::size_t fs = 0;
    for (auto m : std::meta::members_of(
             ^^re::Plain, std::meta::access_context::unchecked()))
      if (std::meta::has_identifier(m) && std::meta::identifier_of(m) == "f")
        ++fs;
    if (fs != 1)
      __builtin_abort();
  }
  return 0;
}
