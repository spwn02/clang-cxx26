//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20
// ADDITIONAL_COMPILE_FLAGS: -freflection

// XFAIL: *

// <meta>
//
// [meta.reflection.substitute]: "If forming Z<TARG-SPLICE(Args)...> leads to a
// failure outside of the immediate context, the program is ill-formed."
//
// The example of the draft: fn2's return type must be deduced, which needs the
// instantiation of its body, and the static_assert in it fails.
//
// FIXME(spwn02/clang-cxx26#125): the fork runs the substitution with its
// diagnostics suppressed and then accepts fn2<int>, so this program compiles
// without any diagnostic. Remove the XFAIL when it is diagnosed.

#include <meta>

using namespace std::meta;

template <typename T>
auto fn2() {
  static_assert(^^T != ^^int); // expected-error {{static assertion failed}}
  return 0;
}

// fn2<int> is instantiated once, so the static_assert fires once.
constexpr bool r2 = can_substitute(^^fn2, {^^int}); // expected-note {{requested here}}
