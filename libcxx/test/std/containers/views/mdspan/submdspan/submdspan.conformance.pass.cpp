//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <mdspan>

// [mdspan.sub]: submdspan, subextents, canonical_slices and the submdspan_mapping specializations of the layout
// mappings (collapsed dimensions, padded result layouts, empty end slices, canonical slices).

#include <cassert>
#include <concepts>
#include <cstddef>
#include <mdspan>
#include <numeric>
#include <tuple>
#include <type_traits>
#include <utility>

template <class T, class U>
constexpr bool same = std::is_same_v<T, U>;

// submdspan_mapping is a hidden friend of the layout mappings, found by argument-dependent lookup only.
template <class M, class... S>
concept has_submdspan_mapping = requires(const M& m, S... s) { submdspan_mapping(m, s...); };
template <class E, class... S>
concept has_subextents = requires(E e, S... s) { std::subextents(e, s...); };
template <class E, class... S>
concept has_canonical_slices = requires(E e, S... s) { std::canonical_slices(e, s...); };

void test_collapsed_dimensions() {
  int a[27];
  std::iota(a, a + 27, 0);
  std::mdspan<int, std::extents<int, 3, 3, 3>> m(a);

  auto middle = std::submdspan(m, std::full_extent, 0, std::full_extent);
  assert(middle.stride(0) == 9 && middle.stride(1) == 1);
  assert((middle[1, 1] == m[1, 0, 1]) && middle[1, 1] == 10);

  auto last = std::submdspan(m, std::full_extent, std::full_extent, 1);
  assert(last.stride(0) == 9 && last.stride(1) == 3);
  assert((last[1, 1] == m[1, 1, 1]) && last[1, 1] == 13);

  std::mdspan<int, std::extents<int, 3, 3, 3>, std::layout_left> l(a);
  auto lmiddle = std::submdspan(l, std::full_extent, 0, std::full_extent);
  assert(lmiddle.stride(0) == 1 && lmiddle.stride(1) == 9);
  assert((lmiddle[1, 1] == l[1, 0, 1]));
  auto lfirst = std::submdspan(l, 0, std::full_extent, std::full_extent);
  assert(lfirst.stride(0) == 3 && lfirst.stride(1) == 9);
  assert((lfirst[1, 1] == l[0, 1, 1]));

  // Every element of every rank-3 collapse agrees with the source.
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j) {
      assert((std::submdspan(m, i, j, std::full_extent)[2] == m[i, j, 2]));
      assert((std::submdspan(m, i, std::full_extent, j)[2] == m[i, 2, j]));
      assert((std::submdspan(m, std::full_extent, i, j)[2] == m[2, i, j]));
      assert((std::submdspan(l, i, j, std::full_extent)[2] == l[i, j, 2]));
      assert((std::submdspan(l, i, std::full_extent, j)[2] == l[i, 2, j]));
      assert((std::submdspan(l, std::full_extent, i, j)[2] == l[2, i, j]));
    }
}

void test_result_layouts() {
  using E46 = std::extents<int, 4, 6>;

  // layout_right: a rectangular unit-stride partial slice keeps rows of the original width.
  {
    int a[24] = {};
    std::mdspan<int, E46> m(a);
    auto s = std::submdspan(m, std::pair{1, 3}, std::pair{1, 4});
    static_assert(same<typename decltype(s)::layout_type, std::layout_right_padded<6>>);
    assert(s.extent(0) == 2 && s.extent(1) == 3 && s.stride(0) == 6 && s.stride(1) == 1);
    assert(s.data_handle() == a + 7);
    // full trailing dimension: plain layout_right
    auto t = std::submdspan(m, std::pair{1, 3}, std::full_extent);
    static_assert(same<typename decltype(t)::layout_type, std::layout_right>);
    assert(t.data_handle() == a + 6);
  }
  // layout_left mirror image
  {
    int a[24] = {};
    std::mdspan<int, E46, std::layout_left> m(a);
    auto s = std::submdspan(m, std::pair{1, 3}, std::pair{1, 4});
    static_assert(same<typename decltype(s)::layout_type, std::layout_left_padded<4>>);
    assert(s.extent(0) == 2 && s.extent(1) == 3 && s.stride(0) == 1 && s.stride(1) == 4);
    assert(s.data_handle() == a + 1 + 4);
    auto t = std::submdspan(m, std::full_extent, std::pair{1, 3});
    static_assert(same<typename decltype(t)::layout_type, std::layout_left>);
  }
  // strided slices fall back to layout_stride
  {
    int a[24] = {};
    std::mdspan<int, E46> m(a);
    auto s = std::submdspan(m, std::extent_slice{0, std::cw<2>, std::cw<2>}, std::full_extent);
    static_assert(same<typename decltype(s)::layout_type, std::layout_stride>);
    assert(s.stride(0) == 12 && s.stride(1) == 1);
  }
  // padded layouts keep their padding
  {
    int a[32] = {};
    std::mdspan<int, E46, std::layout_right_padded<8>> m(a);
    auto s = std::submdspan(m, std::full_extent, std::full_extent);
    static_assert(same<typename decltype(s)::layout_type, std::layout_right_padded<8>>);
    assert(s.stride(0) == 8);
    auto t = std::submdspan(m, std::pair{1, 3}, std::pair{1, 4});
    static_assert(same<typename decltype(t)::layout_type, std::layout_right_padded<8>>);
    assert(t.stride(0) == 8 && t.data_handle() == a + 9);
    auto u = std::submdspan(m, 2, std::pair{1, 4});
    static_assert(same<typename decltype(u)::layout_type, std::layout_right>);
    assert(u.data_handle() == a + 17 && u.extent(0) == 3);

    int b[32] = {};
    std::mdspan<int, E46, std::layout_left_padded<8>> l(b);
    auto v = std::submdspan(l, std::full_extent, std::full_extent);
    static_assert(same<typename decltype(v)::layout_type, std::layout_left_padded<8>>);
    auto w = std::submdspan(l, std::pair{1, 3}, std::pair{1, 4});
    static_assert(same<typename decltype(w)::layout_type, std::layout_left_padded<8>>);
    assert(w.stride(1) == 8 && w.data_handle() == b + 1 + 8);
  }
  // rank-one padded mappings: [mdspan.sub.map.leftpad]/2 and [mdspan.sub.map.rightpad]/2 return layout_left and
  // layout_right respectively for every slice (an LWG candidate for strided slices, which lose their stride).
  {
    int a[8] = {};
    std::mdspan<int, std::extents<int, 8>, std::layout_left_padded<4>> m(a);
    auto s = std::submdspan(m, std::pair{1, 5});
    static_assert(same<typename decltype(s)::layout_type, std::layout_left>);
    auto t = std::submdspan(m, std::extent_slice{0, std::cw<4>, std::cw<2>});
    static_assert(same<typename decltype(t)::layout_type, std::layout_left>);
    std::mdspan<int, std::extents<int, 8>, std::layout_right_padded<4>> r(a);
    auto u = std::submdspan(r, std::extent_slice{0, std::cw<4>, std::cw<2>});
    static_assert(same<typename decltype(u)::layout_type, std::layout_right>);
  }
  // rank-zero results
  {
    int a[24];
    std::iota(a, a + 24, 0);
    std::mdspan<int, E46, std::layout_right_padded<8>> m(a);
    auto s = std::submdspan(m, 1, 2);
    static_assert(s.rank() == 0);
    static_assert(same<typename decltype(s)::layout_type, std::layout_right>);
    assert(s[] == 10);
  }
}

constexpr bool test_empty_end_slice() {
  std::layout_right::mapping<std::extents<int, 4, 6>> m;
  // A lower bound equal to its extent selects required_span_size(), not an out-of-bounds index.
  auto r = submdspan_mapping(m, std::full_extent, std::extent_slice{6, 0, 1});
  assert(r.offset == 24);
  assert(r.mapping.extents().extent(1) == 0);
  auto r2 = submdspan_mapping(m, std::extent_slice{4, 0, 1}, std::full_extent);
  assert(r2.offset == 24);
  return true;
}

constexpr bool test_canonical_slices() {
  // runtime stride of a range slice is kept
  {
    auto t = std::canonical_slices(std::extents<int, 6>{}, std::range_slice{1, 6, 2});
    auto s = std::get<0>(t);
    assert(s.offset == 1 && s.extent == 3 && s.stride == 2);
  }
  // empty range with a constant zero span: stride one
  {
    auto t = std::canonical_slices(std::extents<int, 6>{}, std::range_slice{std::cw<2>, std::cw<2>, 3});
    using S = std::tuple_element_t<0, decltype(t)>;
    static_assert(same<typename S::stride_type, std::constant_wrapper<1>>);
    static_assert(same<typename S::extent_type, std::constant_wrapper<0>>);
  }
  // constant range slices with constant stride
  {
    auto t = std::canonical_slices(std::extents<int, 12>{}, std::range_slice{std::cw<1>, std::cw<11>, std::cw<3>});
    using S = std::tuple_element_t<0, decltype(t)>;
    static_assert(same<typename S::extent_type, std::constant_wrapper<4>>);
    static_assert(S::extent_type::value == 4 && S::stride_type::value == 3);
  }
  // pair-like slices and plain indices
  {
    auto t = std::canonical_slices(std::extents<unsigned, 6, 7>{}, std::pair{1, 4}, 2);
    auto s = std::get<0>(t);
    assert(s.offset == 1u && s.extent == 3u);
    static_assert(same<std::tuple_element_t<1, decltype(t)>, unsigned>);
  }
  // integral-constant-like slices become constant_wrapper
  {
    auto t = std::canonical_slices(std::extents<int, 6>{}, std::integral_constant<long, 3>{});
    static_assert(same<std::tuple_element_t<0, decltype(t)>, std::constant_wrapper<3>>);
    static_assert(same<std::tuple_element_t<0, decltype(t)>, std::constant_wrapper<3, int>>);
  }
  return true;
}

constexpr bool test_subextents() {
  using E = std::extents<int, 4, std::dynamic_extent, 8>;
  E e(4, 10, 8);
  auto se = std::subextents(e, std::full_extent, std::range_slice{1, 9, std::cw<2>}, 3);
  static_assert(decltype(se)::rank() == 2);
  static_assert(decltype(se)::static_extent(0) == 4);
  assert(se.extent(1) == 4);
  auto s2 = std::subextents(e, 1, std::extent_slice{std::cw<0>, std::cw<5>, std::cw<1>}, std::pair{2, 5});
  static_assert(decltype(s2)::rank() == 2);
  static_assert(decltype(s2)::static_extent(0) == 5);
  static_assert(decltype(s2)::static_extent(1) == std::dynamic_extent);
  assert(s2.extent(0) == 5 && s2.extent(1) == 3);
  auto s3 = std::subextents(e, 0, 0, 0);
  static_assert(decltype(s3)::rank() == 0);
  return true;
}

void test_slice_requirements() {
  // slice types are checked by the extents' index type
  static_assert(has_subextents<std::extents<int, 3>, std::constant_wrapper<2>>);
  static_assert(!has_subextents<std::extents<int, 3>>);
  static_assert(!has_canonical_slices<std::extents<int, 3, 3>, int>);
  using M = std::layout_right::mapping<std::extents<int, 3, 3>>;
  static_assert(has_submdspan_mapping<M, std::full_extent_t, std::full_extent_t>);
  static_assert(!has_submdspan_mapping<M, std::full_extent_t>);
  static_assert(std::default_initializable<std::extent_slice<int, int, int>>);
  static_assert(std::default_initializable<std::range_slice<int, int>>);
  static_assert(same<decltype(std::range_slice<int, int>::stride), std::constant_wrapper<1zu>>);
  static_assert(std::is_aggregate_v<std::submdspan_mapping_result<M>>);
}

int main(int, char**) {
  test_collapsed_dimensions();
  test_result_layouts();
  test_empty_end_slice();
  static_assert(test_empty_end_slice());
  test_canonical_slices();
  static_assert(test_canonical_slices());
  test_subextents();
  static_assert(test_subextents());
  test_slice_requirements();
  return 0;
}
