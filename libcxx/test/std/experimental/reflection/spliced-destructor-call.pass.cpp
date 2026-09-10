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
// Regression test for upstream bloomberg/clang-p2996 issue #150: an explicit
// destructor call naming its class type via a splice-type-specifier
// ('value.~typename[:R:]()') failed to parse at all. See
// clang/test/Reflection/issue150-spliced-destructor.cpp for the fuller
// parse/Sema-level coverage (including the negative cases); this test
// specifically proves the destructor genuinely runs at runtime, in both a
// non-dependent and a dependent (function template) context.

#include <cassert>
#include <meta>
#include <new>

struct test {
  bool* flag;
  ~test() { *flag = true; }
};

template <typename T>
void destroy(T& value) {
  value.~typename[:^^T:]();
}

int main(int, char**) {
  bool destructed = false;
  {
    test value{&destructed};
    value.~typename[:^^test:]();
    assert(destructed);
    // Placement-construct a fresh object into the same storage so this
    // block's own (now-redundant) automatic destruction at scope exit
    // doesn't double-destroy an already-destroyed object.
    ::new (static_cast<void*>(&value)) test{&destructed};
  }

  destructed = false;
  {
    test value{&destructed};
    destroy(value); // dependent context: T is deduced, splice is on T.
    assert(destructed);
    ::new (static_cast<void*>(&value)) test{&destructed};
  }

  return 0;
}
