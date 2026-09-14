// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
#include <__mdspan/submdspan.h>
#include <cassert>
#include <tuple>

int main() {
  using E = std::extents<int, 4, std::dynamic_extent, 8>;
  E e(4, 10, 8);
  auto c = std::canonical_slices(e, std::full_extent, std::range_slice{1, 9, std::cw<2>},
                                 std::extent_slice{1, std::cw<3>, std::cw<2>});
  assert(std::is_same_v<decltype(std::get<0>(c)), std::full_extent_t&>);
  assert(std::get<1>(c).extent == 4);
  assert(std::get<2>(c).stride == 2);
  auto se = std::subextents(e, std::full_extent, std::range_slice{1, 9, std::cw<2>},
                            std::extent_slice{1, std::cw<3>, std::cw<2>});
  static_assert(decltype(se)::static_extent(0) == 4);
  static_assert(decltype(se)::static_extent(1) == std::dynamic_extent);
  static_assert(decltype(se)::static_extent(2) == 3);
  assert(se.extent(1) == 4);

  using DE = std::extents<int, std::dynamic_extent, 8>;
  DE de(7, 8);
  auto dse = std::subextents(de, std::full_extent, std::full_extent);
  static_assert(decltype(dse)::static_extent(0) == std::dynamic_extent);
  static_assert(decltype(dse)::static_extent(1) == 8);
  assert(dse.extent(0) == 7);
}
