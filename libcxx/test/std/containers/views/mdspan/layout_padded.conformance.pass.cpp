//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <mdspan>

// [mdspan.layout.leftpad], [mdspan.layout.rightpad]: layout_left_padded::mapping and
// layout_right_padded::mapping as specified by P2642R6 (and the P3222R0 conversions).

#include <array>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <mdspan>
#include <type_traits>

constexpr auto dyn = std::dynamic_extent;

template <class M, class... I>
concept callable = requires(const M& m, I... i) { m(i...); };

constexpr bool test() {
  using E3  = std::extents<int, 3, 4, 5>;
  using E2  = std::extents<int, 4, 3>;
  using DE2 = std::dextents<int, 2>;

  // layout_left_padded / layout_right_padded default their padding value to dynamic_extent.
  static_assert(std::same_as<std::layout_left_padded<>, std::layout_left_padded<dyn>>);
  static_assert(std::same_as<std::layout_right_padded<>, std::layout_right_padded<dyn>>);

  // Strides, spans and the layout equations.
  {
    std::layout_left_padded<8>::mapping<E3> l;
    assert(l.stride(0) == 1 && l.stride(1) == 8 && l.stride(2) == 32);
    assert((l.strides() == std::array<int, 3>{1, 8, 32}));
    assert(l.required_span_size() == 155 && l(2, 3, 4) == 154);
    assert(!l.is_exhaustive());
    std::layout_right_padded<8>::mapping<E3> r;
    assert(r.stride(2) == 1 && r.stride(1) == 8 && r.stride(0) == 32);
    assert((r.strides() == std::array<int, 3>{32, 8, 1}));
    assert(r.required_span_size() == 93 && r(2, 3, 4) == 92);
    assert(!r.is_exhaustive());
  }
  // The required span size of an empty index space is zero (it was a negative number).
  {
    std::layout_left_padded<4>::mapping<std::extents<int, 2, 0>> l;
    std::layout_right_padded<4>::mapping<std::extents<int, 0, 2>> r;
    assert(l.required_span_size() == 0 && r.required_span_size() == 0);
    std::layout_left_padded<4>::mapping<std::extents<int, 0, 5>> l2;
    assert(l2.required_span_size() == 0);
  }
  // is_always_exhaustive / is_exhaustive.
  static_assert(std::layout_left_padded<4>::mapping<E2>::is_always_exhaustive());
  static_assert(!std::layout_left_padded<8>::mapping<E2>::is_always_exhaustive());
  static_assert(!std::layout_left_padded<dyn>::mapping<E2>::is_always_exhaustive());
  static_assert(std::layout_left_padded<8>::mapping<std::extents<int, 4>>::is_always_exhaustive());
  static_assert(std::layout_left_padded<8>::mapping<std::extents<int>>::is_always_exhaustive());
  static_assert(std::layout_right_padded<3>::mapping<std::extents<int, 4, 3>>::is_always_exhaustive());
  static_assert(!std::layout_right_padded<8>::mapping<std::extents<int, 4, 3>>::is_always_exhaustive());
  {
    std::layout_left_padded<dyn>::mapping<DE2> m(DE2(4, 3), 4);
    assert(m.is_exhaustive());
    std::layout_left_padded<dyn>::mapping<DE2> n(DE2(4, 3), 8);
    assert(!n.is_exhaustive());
  }
  // Equality compares the extents and the padding stride.
  {
    std::layout_left_padded<dyn>::mapping<DE2> a(DE2(4, 3), 4), b(DE2(4, 3), 8), c(DE2(4, 3), 4);
    assert(a == c && a != b);
    std::layout_left_padded<4>::mapping<E2> s;
    assert(s == a && s != b);
    std::layout_right_padded<dyn>::mapping<DE2> x(DE2(3, 4), 4), y(DE2(3, 4), 8);
    assert(x != y && x == x);
  }
  // Same-orientation conversions and their explicitness.
  {
    using L8 = std::layout_left_padded<8>::mapping<E3>;
    using LD = std::layout_left_padded<dyn>::mapping<E3>;
    static_assert(std::is_convertible_v<L8, LD>);
    static_assert(std::is_constructible_v<L8, LD> && !std::is_convertible_v<LD, L8>);
    LD d(L8{});
    assert(d.stride(1) == 8);
    using R8 = std::layout_right_padded<8>::mapping<E3>;
    using RD = std::layout_right_padded<dyn>::mapping<E3>;
    static_assert(std::is_convertible_v<R8, RD>);
    static_assert(std::is_constructible_v<R8, RD> && !std::is_convertible_v<RD, R8>);
    // Different padding values are rejected by Mandates, not by SFINAE: check the constraint side.
    static_assert(std::is_constructible_v<std::layout_left_padded<4>::mapping<E3>,
                                          const std::layout_left_padded<4>::mapping<E3>&>);
  }
  // Opposite-orientation conversions exist for rank <= 1 only.
  {
    using E1 = std::extents<int, 5>;
    using L  = std::layout_left_padded<4>::mapping<E1>;
    using R  = std::layout_right_padded<8>::mapping<E1>;
    static_assert(std::is_convertible_v<R, L>);
    static_assert(std::is_convertible_v<L, R>);
    static_assert(!std::is_constructible_v<std::layout_left_padded<4>::mapping<E2>,
                                           std::layout_right_padded<4>::mapping<E2>>);
    static_assert(std::is_constructible_v<L, std::layout_right::mapping<E1>>);
    assert(L(R{}).required_span_size() == 5);
  }
  // layout_stride conversions: rank 0 is implicit, higher ranks are explicit.
  {
    using S0 = std::layout_stride::mapping<std::extents<int>>;
    static_assert(std::is_convertible_v<S0, std::layout_left_padded<4>::mapping<std::extents<int>>>);
    using S2 = std::layout_stride::mapping<E2>;
    static_assert(!std::is_convertible_v<S2, std::layout_left_padded<4>::mapping<E2>>);
    static_assert(std::is_constructible_v<std::layout_left_padded<4>::mapping<E2>, S2>);
    S2 s(E2{}, std::array<int, 2>{1, 4});
    std::layout_left_padded<4>::mapping<E2> from(s);
    assert(from.stride(1) == 4);
    std::layout_stride::mapping<E2> back(from);
    assert(back.stride(1) == 4);
  }
  // layout_left / layout_right can be constructed from an exhaustive padded mapping.
  {
    std::layout_left_padded<4>::mapping<E2> pl;
    std::layout_left::mapping<E2> l(pl);
    assert(l.required_span_size() == pl.required_span_size());
    std::layout_right_padded<3>::mapping<std::extents<int, 4, 3>> pr;
    std::layout_right::mapping<std::extents<int, 4, 3>> r(pr);
    assert(r.required_span_size() == pr.required_span_size());
  }
  // Index call operator constraints.
  {
    using M = std::layout_left_padded<4>::mapping<DE2>;
    static_assert(callable<M, int, int>);
    static_assert(!callable<M, int>);
    static_assert(!callable<M, int, int, int>);
    struct throwing {
      operator int() const { return 0; }
    };
    static_assert(!callable<M, int, throwing>);
  }
  return true;
}

int main(int, char**) {
  test();
  static_assert(test());
  return 0;
}
