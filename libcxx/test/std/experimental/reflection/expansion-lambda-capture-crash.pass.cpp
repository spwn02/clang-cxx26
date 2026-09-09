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

// <experimental/reflection>
//
// [reflection]
//
// Regression test for upstream bloomberg/clang-p2996 issue #169: a
// generic lambda with a captured local, nested inside a consteval function
// template, whose body contains a `template for` iterable expansion, used
// to crash Sema. The crash traced to CheckIfAnyEnclosingLambdasMustCaptureAnyPotentialCaptures
// assuming LambdaScopeInfo::CallOperator stays synchronized with
// Sema::CurContext -- an expansion statement can leave the lambda's scope
// on the FunctionScopes stack while CurContext has moved to the enclosing
// expansion context, desynchronizing the two.

#include <meta>

namespace n {}

template <std::meta::info R> consteval int f() {
  int i = 0;
  return [&]<int x>() {
    template for (constexpr std::meta::info member :
                  std::define_static_array(std::meta::members_of(
                      R, std::meta::access_context::unchecked())))
      i += 1;
    return i;
  }.template operator()<0>();
}

static_assert(f<^^n>() == 0);

int main() {
  return 0;
}
