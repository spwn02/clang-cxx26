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

  int data[4 * 5 * 6]{};
  std::mdspan view(data, e);
  auto sub = std::submdspan(view, std::full_extent, std::extent_slice{1, std::cw<3>, std::cw<2>}, 2);
  static_assert(std::is_same_v<typename decltype(sub)::layout_type, std::layout_stride>);
  assert(sub.data_handle() == data + 8);
  assert(sub.extent(0) == 4);
  assert(sub.extent(1) == 3);

  auto full = std::submdspan_mapping(m, std::full_extent, std::full_extent, std::full_extent);
  static_assert(std::is_same_v<typename decltype(full.mapping)::layout_type, std::layout_right>);

  std::layout_right::mapping<E> right(E{});
  auto right_contiguous =
      std::submdspan_mapping(right, std::extent_slice{1, std::cw<3>, std::cw<1>}, std::full_extent, std::full_extent);
  static_assert(std::is_same_v<typename decltype(right_contiguous.mapping)::layout_type, std::layout_right>);
  assert(right_contiguous.offset == 30);
  assert(right_contiguous.mapping.stride(0) == 30);

  std::layout_left::mapping<E> left(E{});
  auto left_contiguous =
      std::submdspan_mapping(left, std::full_extent, std::full_extent, std::extent_slice{1, std::cw<3>, std::cw<1>});
  static_assert(std::is_same_v<typename decltype(left_contiguous.mapping)::layout_type, std::layout_left>);
  assert(left_contiguous.offset == 20);
  assert(left_contiguous.mapping.stride(1) == 4);

  auto singleton = std::submdspan_mapping(
      right, std::full_extent, std::full_extent, std::extent_slice{1, std::cw<1>, std::cw<2>});
  static_assert(std::is_same_v<typename decltype(singleton.mapping)::layout_type, std::layout_stride>);
  assert(singleton.mapping.stride(2) == 1);
}
