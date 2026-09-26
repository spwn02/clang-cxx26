//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20
// ADDITIONAL_COMPILE_FLAGS: -freflection

// <meta>
//
// [meta.reflection.substitute]: "If forming Z<TARG-SPLICE(Args)...> leads to a
// failure outside of the immediate context, the program is ill-formed."
//
// The example of the draft: fn2's return type must be deduced, which needs the
// instantiation of its body, and the static_assert in it fails.
//
// (spwn02/clang-cxx26#125) The substitution itself runs with diagnostics suppressed, but the
// body instantiation needed for return type deduction does not: its errors are hard errors.

#include <meta>

using namespace std::meta;

template <typename T>
auto fn2() {
  static_assert(^^T != ^^int); // expected-error {{static assertion failed}}
  return 0;
}

// fn2<int> is instantiated once, so the static_assert fires once.
constexpr bool r2 = can_substitute(^^fn2, {^^int}); // expected-note {{requested here}}

// A specialization whose body is fine is still accepted, and does not re-diagnose.
constexpr bool r3 = can_substitute(^^fn2, {^^long});
constexpr auto r4 = substitute(^^fn2, {^^long});
