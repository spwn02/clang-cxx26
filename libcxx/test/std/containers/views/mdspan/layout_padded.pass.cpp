// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
#include <cassert>
#include <mdspan>

int main() {
  using E = std::extents<int, 3, 4, 5>;
  std::layout_left_padded<8>::mapping lm(E{});
  assert(lm.stride(0) == 1 && lm.stride(1) == 8 && lm.stride(2) == 32);
  assert(lm(2, 3, 4) == 154 && lm.required_span_size() == 155);
  assert(!lm.is_exhaustive());
  std::layout_right_padded<8>::mapping rm(E{});
  assert(rm.stride(2) == 1 && rm.stride(1) == 8 && rm.stride(0) == 32);
  assert(rm(2, 3, 4) == 92 && rm.required_span_size() == 93);
  assert(!rm.is_exhaustive());
  std::layout_left_padded<std::dynamic_extent>::mapping dl(std::extents<int, std::dynamic_extent, 4, 5>(3), 7);
  assert(dl.stride(1) == 7);
}
