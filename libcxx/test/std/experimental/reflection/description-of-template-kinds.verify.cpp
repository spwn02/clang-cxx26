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

// FIXME(spwn02/clang-cxx26#126): the failure reason is no longer part of the
// diagnostic, only the generic "exception thrown here was not caught" note,
// so that is what the expectations below check.

#include <experimental/meta>

consteval std::meta::info find_builtin_template() {
  for (auto m : std::meta::members_of(
           ^^::, std::meta::access_context::unchecked()))
    if (std::meta::is_template(m) && std::meta::has_identifier(m) &&
        std::meta::identifier_of(m) == "__make_integer_seq")
      return m;
  return ^^void;
}
static_assert(find_builtin_template() != ^^void);

constexpr auto r = std::meta::return_type_of(find_builtin_template());
// expected-error@-1 {{must be initialized by a constant expression}}
// expected-note@* {{exception thrown here was not caught within the constant expression}}
// expected-note@-3 {{in call to}}
