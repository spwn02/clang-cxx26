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
// CWG 3111 (array-type template parameter objects): 'std::meta::reflect_constant'
// on an array-typed argument must not be silently unreachable/incorrect —
// per the DR, it routes through the same 'reflect_constant_array' mechanism
// used for an explicit call. Also covers the fix that made this reachable at
// all: 'reflect_constant' previously took its argument by value, which
// array-to-pointer-adjusts an array-typed template parameter before
// 'is_array_type' can ever see it; it now takes 'const T&', matching both
// this header's own synopsis and the standard's declared signature.
//
// LWG 4432 (element init for 'reflect_constant_array'): each element is
// 'static_cast<T>(*it)' before being reflected, so a range whose reference
// type isn't plain 'T&' (a proxy reference, e.g. 'vector<bool>') converts to
// the array's element type rather than deducing 'reflect_constant's template
// parameter from the proxy type itself.

#include <meta>
#include <vector>

                              // ================
                              // CWG 3111: direct
                              // reflect_constant
                              // on an array
                              // ================

namespace direct_array {
constexpr int arr[3] = {1, 2, 3};
constexpr auto r = std::meta::reflect_constant(arr);
static_assert(std::meta::is_array_type(std::meta::type_of(r)));
static_assert(std::meta::extract<const int *>(r)[0] == 1);
static_assert(std::meta::extract<const int *>(r)[1] == 2);
static_assert(std::meta::extract<const int *>(r)[2] == 3);

// Matches calling 'reflect_constant_array' directly on the same contents.
constexpr auto via_array_fn =
    std::meta::reflect_constant_array(std::vector<int>{1, 2, 3});
static_assert(std::meta::extract<const int *>(via_array_fn)[0] ==
              std::meta::extract<const int *>(r)[0]);
}  // namespace direct_array

                              // ================
                              // CWG 3111: nested
                              // (multi-dimensional)
                              // arrays remain
                              // unsupported
                              // ================
//
// 'reflect_constant' on a multi-dimensional array (e.g. 'int[2][2]') is not
// fixed by this change: the backing 'reflect_constant_array' -> 'substitute'
// mechanism can't yet represent an array-typed non-type template parameter
// pack element. This is intentionally excluded via the requires-clause
// (SFINAE), matching this function's pre-existing behavior for every array
// type before this fix (a clean "no matching function", not a hard
// substitution-failure deep in 'reflect_constant_array'). See the design
// comments on 'reflect_constant' in libcxx/include/meta for the full
// story; a follow-up needs a different backing representation for nested
// arrays specifically.

                              // ================
                              // CWG 3111: array of
                              // a structural class
                              // type still works
                              // ================

namespace class_element_array {
struct Point {
  int x;
  int y;
  friend consteval bool operator==(Point, Point) = default;
};
constexpr Point pts[2] = {{1, 2}, {3, 4}};
constexpr auto r = std::meta::reflect_constant(pts);
static_assert(std::meta::is_array_type(std::meta::type_of(r)));
static_assert(std::meta::extract<const Point *>(r)[0] == Point{1, 2});
static_assert(std::meta::extract<const Point *>(r)[1] == Point{3, 4});
}  // namespace class_element_array

                              // ================
                              // LWG 4432: proxy
                              // reference range
                              // (vector<bool>)
                              // ================

namespace proxy_reference {
consteval bool test() {
  std::vector<bool> bits = {true, false, true};
  auto r = std::meta::reflect_constant_array(bits);
  auto *arr = std::meta::extract<const bool *>(r);
  return arr[0] == true && arr[1] == false && arr[2] == true;
}
static_assert(test());
}  // namespace proxy_reference

int main() { return 0; }
