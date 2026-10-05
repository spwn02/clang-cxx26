//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <mdspan>

// [mdspan.sub.range.slices], [mdspan.sub.helpers], [mdspan.sub.canonical], [mdspan.sub.extents]: Mandates of the
// slice types and of the canonicalization of constant slices.

#include <mdspan>

// expected-note@*:* 0+ {{in instantiation of}}

void slice_member_types() {
  // expected-error-re@*:* {{static assertion failed {{.*}}extent_slice: OffsetType, ExtentType and StrideType must be signed or unsigned integer types}}
  [[maybe_unused]] std::extent_slice<double, int, int> e;
  // expected-error-re@*:* {{static assertion failed {{.*}}range_slice: FirstType, LastType and StrideType must be signed or unsigned integer types}}
  [[maybe_unused]] std::range_slice<int, double> r;
}

void static_slice_out_of_range() {
  // the constant collapsing index must be less than the static extent
  // expected-error-re@*:* {{static assertion failed {{.*}}every canonical slice must be a valid submdspan slice type}}
  (void)std::subextents(std::extents<int, 3>{}, std::cw<4>);
  // offset + 1 + (extent - 1) * stride must not exceed the static extent
  // expected-error-re@*:* {{static assertion failed {{.*}}every canonical slice must be a valid submdspan slice type}}
  (void)std::canonical_slices(std::extents<int, 5>{}, std::extent_slice{std::cw<1>, std::cw<3>, std::cw<2>});
}

void unrepresentable_constant_index() {
  // expected-error-re@*:* {{static assertion failed {{.*}}constant slice index must be representable}}
  (void)std::canonical_slices(std::extents<signed char, std::dynamic_extent>{3}, std::cw<256>);
}

struct NotASlice {};

void not_a_slice_type() {
  // expected-error-re@*:* 2 {{static assertion failed {{.*}}slice specifier must be a submdspan slice type}}
  (void)std::canonical_slices(std::extents<int, 3>{}, NotASlice{});
}
