//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <type_traits>

// template<class T>
//   consteval bool is_within_lifetime(const T* p) noexcept; // C++26

#include <type_traits>

#include "test_macros.h"

#ifndef __cpp_lib_is_within_lifetime
#  if TEST_HAS_BUILTIN(__builtin_is_within_lifetime)
#    error __cpp_lib_is_within_lifetime should be defined
#  endif
#else

// Test the signature
static_assert(noexcept(std::is_within_lifetime((const int*)nullptr)));
static_assert(std::is_same_v<decltype(std::is_within_lifetime((const int*)nullptr)), bool>);

// Per [meta.const.eval]p4, a call to `is_within_lifetime` is ill-formed
// during the evaluation of an expression E as a core constant expression
// unless p points to an object usable in constant expressions or whose
// complete object's lifetime began within E. Empirically (confirmed by
// direct compiler probing, not inferred from the wording alone), E here is
// scoped to a *consteval* (immediate-function) invocation boundary: an
// ordinary `constexpr` helper function evaluated only incidentally as part
// of a `static_assert` does not count, even though the overall
// `static_assert` expression is manifestly constant-evaluated. Every helper
// below that creates the object it inspects must therefore itself be
// `consteval`, not merely `constexpr`, matching the pattern used by
// clang/test/SemaCXX/builtin-is-within-lifetime.cpp for the underlying
// builtin.

consteval bool test_scalar() {
  int i = 0;
  return std::is_within_lifetime(&i);
}
static_assert(test_scalar());

consteval bool test_union_alternative() {
  union {
    int i;
    char c;
  } u{.i = 0};
  if (!std::is_within_lifetime(&u.i))
    return false;
  if (std::is_within_lifetime(&u.c))
    return false;
  u.c = '\0'; // begins the lifetime of the `char` alternative
  if (std::is_within_lifetime(&u.i))
    return false;
  if (!std::is_within_lifetime(&u.c))
    return false;
  return true;
}
static_assert(test_union_alternative());

struct TrivialPoint {
  int x;
  int y;
};

consteval bool test_class_type() {
  TrivialPoint p{1, 2};
  return std::is_within_lifetime(&p);
}
static_assert(test_class_type());

consteval bool test_void_pointer() {
  int i = 0;
  return std::is_within_lifetime(static_cast<const void*>(&i));
}
static_assert(test_void_pointer());

#endif // __cpp_lib_is_within_lifetime

int main(int, char**) { return 0; }
