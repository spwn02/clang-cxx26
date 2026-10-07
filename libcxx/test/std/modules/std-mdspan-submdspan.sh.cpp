//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: clang-modules-build
// UNSUPPORTED: gcc

// XFAIL: has-no-cxx-module-support

// RUN: mkdir %t
// RUN: %{cxx} %{compile_flags} -std=c++26 -freflection-latest \
// RUN:     -Wno-reserved-module-identifier -Wno-reserved-user-defined-literal \
// RUN:     --precompile -o %t/std.pcm -c %{module-dir}/std.cppm
// RUN: %{cxx} %{compile_flags} %{link_flags} -std=c++26 -freflection-latest \
// RUN:     -fmodule-file=std=%t/std.pcm %t/std.pcm \
// RUN:     %s -o %t/std-mdspan-submdspan.sh.cpp.tsk
// RUN: %{exec} %t/std-mdspan-submdspan.sh.cpp.tsk

// [mdspan.layout.leftpad], [mdspan.layout.rightpad], [mdspan.sub]: the padded layouts and the submdspan facilities
// are exported by the std module; submdspan_mapping is found by argument-dependent lookup only.
#include <cassert>

import std;

// A layout whose mapping derives from layout_right::mapping inherits its hidden friend submdspan_mapping.
struct derived_layout {
  template <class Extents>
  struct mapping : std::layout_right::mapping<Extents> {
    using layout_type = derived_layout;
    using std::layout_right::mapping<Extents>::mapping;
  };
};

// A layout without any submdspan_mapping is not sliceable.
struct plain_layout {
  template <class Extents>
  class mapping {
  public:
    using extents_type = Extents;
    using index_type   = typename Extents::index_type;
    using size_type    = typename Extents::size_type;
    using rank_type    = typename Extents::rank_type;
    using layout_type  = plain_layout;

    constexpr mapping() noexcept = default;
    constexpr mapping(const Extents& e) noexcept : m_(e) {}
    constexpr const extents_type& extents() const noexcept { return m_.extents(); }
    constexpr index_type required_span_size() const noexcept { return m_.required_span_size(); }
    template <class... Indices>
      requires(sizeof...(Indices) == Extents::rank() && (std::is_convertible_v<Indices, index_type> && ...) &&
               (std::is_nothrow_constructible_v<index_type, Indices> && ...))
    constexpr index_type operator()(Indices... idxs) const noexcept {
      return m_(idxs...);
    }
    static constexpr bool is_always_unique() noexcept { return true; }
    static constexpr bool is_always_exhaustive() noexcept { return true; }
    static constexpr bool is_always_strided() noexcept { return true; }
    constexpr bool is_unique() const noexcept { return true; }
    constexpr bool is_exhaustive() const noexcept { return true; }
    constexpr bool is_strided() const noexcept { return true; }
    constexpr index_type stride(rank_type r) const noexcept { return m_.stride(r); }
    friend constexpr bool operator==(const mapping&, const mapping&) noexcept = default;

  private:
    std::layout_right::mapping<Extents> m_;
  };
};

template <class M, class... S>
concept can_submdspan = requires(M m, S... s) { std::submdspan(m, s...); };
template <class M, class... S>
concept can_submdspan_mapping = requires(const M& m, S... s) { submdspan_mapping(m, s...); };

int main(int, char**) {
  using E = std::extents<int, 4, 6>;
  int data[24];
  for (int i = 0; i < 24; ++i)
    data[i] = i;

  // padded layouts, with the default PaddingValue
  [[maybe_unused]] std::layout_left_padded<> lp;
  [[maybe_unused]] std::layout_right_padded<> rp;
  std::layout_right_padded<8>::mapping<E> padded(E{});
  assert(padded.strides()[0] == 8 && padded.required_span_size() == 30);
  assert(padded == padded);

  // submdspan results, canonical slices and subextents
  std::mdspan<int, E> m(data);
  auto rect = std::submdspan(m, std::pair{1, 3}, std::pair{1, 4});
  static_assert(std::same_as<typename decltype(rect)::layout_type, std::layout_right_padded<6>>);
  assert((rect[1, 2] == m[2, 3]));
  auto column = std::submdspan(m, std::full_extent, 2);
  static_assert(column.rank() == 1);
  assert(column.stride(0) == 6 && column[3] == 20);
  auto c = std::canonical_slices(E{}, std::range_slice{1, 6, 2}, std::full_extent);
  assert(std::get<0>(c).stride == 2 && std::get<0>(c).extent == 3);
  auto se = std::subextents(E{}, std::extent_slice{1, std::cw<2>, std::cw<1>}, std::full_extent);
  static_assert(decltype(se)::static_extent(0) == 2);

  // submdspan_mapping is a hidden friend
  std::layout_right::mapping<E> rm(E{});
  auto r = submdspan_mapping(rm, std::full_extent, std::extent_slice{6, 0, 1});
  static_assert(std::same_as<typename decltype(r.mapping)::layout_type, std::layout_stride>);
  assert(r.offset == 24);

  // sliceable and non-sliceable user layouts
  static_assert(can_submdspan<std::mdspan<int, E, derived_layout>, std::full_extent_t, int>);
  static_assert(can_submdspan_mapping<derived_layout::mapping<E>, std::full_extent_t, std::full_extent_t>);
  static_assert(!can_submdspan<std::mdspan<int, E, plain_layout>, std::full_extent_t, int>);
  static_assert(!can_submdspan_mapping<plain_layout::mapping<E>, std::full_extent_t, std::full_extent_t>);
  std::mdspan<int, E, derived_layout> dm(data);
  auto dsub = std::submdspan(dm, 1, std::full_extent);
  assert(dsub[2] == 8);
  return 0;
}
