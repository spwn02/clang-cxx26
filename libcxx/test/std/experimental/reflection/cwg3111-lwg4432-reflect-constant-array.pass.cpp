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
//
// CWG 3111 residual (nested/multi-dimensional arrays): a naive one-
// dimension-at-a-time recursive design for a multi-dimensional array (build
// each row's own backing object, then collect a pack of *references* to
// those row objects as a non-type template argument pack) can't work --
// arrays are never copy-list-initializable from another array object, so
// there's no way to copy referenced rows into a fresh contiguous backing
// array. The actual fix flattens all the way down to the ultimate scalar
// leaf type, gathers every dimension's extent along the way, and
// reconstructs the correctly-nested array type from that flat extent list
// (see 'libcxx/include/meta's 'reflect_constant_array' and
// '__define_static::__nd_array_shape'/'FixedNDArray' for the full design) --
// ordinary aggregate-initialization brace elision then fills the nested
// array correctly from one flat, row-major scalar list, exactly like
// 'int a[2][3] = {1,2,3,4,5,6};' does in non-reflective code.

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
                              // CWG 3111 residual:
                              // nested (multi-
                              // dimensional) arrays
                              // ================

namespace nested_array {
constexpr int arr2d[2][3] = {{1, 2, 3}, {4, 5, 6}};
constexpr auto r2d = std::meta::reflect_constant(arr2d);
static_assert(std::meta::is_array_type(std::meta::type_of(r2d)));
static_assert(std::meta::extract<const int(&)[2][3]>(r2d)[0][0] == 1);
static_assert(std::meta::extract<const int(&)[2][3]>(r2d)[0][2] == 3);
static_assert(std::meta::extract<const int(&)[2][3]>(r2d)[1][1] == 5);

// Matches calling 'reflect_constant_array' directly on the same argument.
constexpr auto via_array_fn_2d = std::meta::reflect_constant_array(arr2d);
static_assert(std::meta::extract<const int(&)[2][3]>(via_array_fn_2d)[1][2] ==
              std::meta::extract<const int(&)[2][3]>(r2d)[1][2]);

// A third dimension recurses the same way, arbitrarily deep.
constexpr int arr3d[2][2][2] = {{{1, 2}, {3, 4}}, {{5, 6}, {7, 8}}};
constexpr auto r3d = std::meta::reflect_constant(arr3d);
static_assert(std::meta::is_array_type(std::meta::type_of(r3d)));
static_assert(std::meta::extract<const int(&)[2][2][2]>(r3d)[0][0][0] == 1);
static_assert(std::meta::extract<const int(&)[2][2][2]>(r3d)[1][1][1] == 8);
static_assert(std::meta::extract<const int(&)[2][2][2]>(r3d)[1][0][1] == 6);

// 'define_static_array' on a nested array returns a 'span' over the
// correctly-shaped backing rows, not just the flat scalar leaves.
consteval bool test_define_static_array_nested() {
  int arr[2][3] = {{1, 2, 3}, {4, 5, 6}};
  auto sp = std::define_static_array(arr);
  return sp.size() == 2 && sp[0][0] == 1 && sp[0][1] == 2 && sp[0][2] == 3 &&
         sp[1][0] == 4 && sp[1][1] == 5 && sp[1][2] == 6;
}
static_assert(test_define_static_array_nested());

// A row consisting of a structural class type (not just scalars) also
// works, since the per-leaf reflection still bottoms out at
// 'reflect_constant's plain (non-array) overload.
struct Point {
  int x;
  int y;
  friend consteval bool operator==(Point, Point) = default;
};
constexpr Point pts2d[2][2] = {{{1, 2}, {3, 4}}, {{5, 6}, {7, 8}}};
constexpr auto pts_r = std::meta::reflect_constant(pts2d);
static_assert(std::meta::extract<const Point(&)[2][2]>(pts_r)[0][1] ==
              Point{3, 4});
static_assert(std::meta::extract<const Point(&)[2][2]>(pts_r)[1][0] ==
              Point{5, 6});
}  // namespace nested_array

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
