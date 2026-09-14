// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
#include <mdspan>
#include <cassert>
#include <type_traits>

int main() {
  using E = std::extents<int, 4, 5, 6>;
  E e;
  std::layout_right::mapping<E> m(e);
  auto r = std::submdspan_mapping(m, std::full_extent, std::extent_slice{1, std::cw<3>, std::cw<2>}, 2);
  using R = decltype(r.mapping);
  static_assert(std::is_same_v<typename R::layout_type, std::layout_stride>);
  static_assert(R::extents_type::rank() == 2);
  static_assert(R::extents_type::static_extent(0) == 4);
  static_assert(R::extents_type::static_extent(1) == 3);
  assert(r.offset == 8);
  assert(r.mapping.extents().extent(0) == 4);
  assert(r.mapping.extents().extent(1) == 3);
  assert(r.mapping.stride(0) == 30);
  assert(r.mapping.stride(1) == 12);
}
