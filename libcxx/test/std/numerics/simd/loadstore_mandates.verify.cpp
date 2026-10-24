//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <simd>

// LWG4420: the Mandates of the loads and stores: the conversion must be value-preserving unless
// flag_convert is given, and ranges::size(r) >= V::size() when it is a constant expression.

#include <array>
#include <simd>

namespace simd = std::simd;
using V        = simd::vec<float, 4>;

std::array<double, 4> d4{};
std::array<float, 2> f2{};

void load_not_value_preserving() {
  (void)simd::partial_load<V>(d4);
  // expected-error@*:* {{the conversion must be value-preserving unless flag_convert is given}}
}

void store_not_value_preserving() {
  simd::partial_store(simd::vec<double, 4>{}, std::array<float, 4>{});
  // expected-error@*:* {{the conversion must be value-preserving unless flag_convert is given}}
}

void unchecked_range_too_small() {
  (void)simd::unchecked_load<V>(f2);
  // expected-error@*:* {{ranges::size(r) must be at least V::size()}}
}

void unchecked_store_range_too_small() {
  simd::unchecked_store(V{}, f2);
  // expected-error@*:* {{ranges::size(r) must be at least V::size()}}
}

void range_constructor_not_value_preserving() {
  (void)V(d4);
  // expected-error@*:* {{the conversion from the range's value type to value_type is not value-preserving}}
  (void)V(d4, V::mask_type(true));
  // expected-error@*:* {{the conversion from the range's value type to value_type is not value-preserving}}
}

void accepted() {
  (void)simd::partial_load<V>(d4, simd::flag_convert);
  simd::partial_store(simd::vec<double, 4>{}, std::array<float, 4>{}, simd::flag_convert);
}
