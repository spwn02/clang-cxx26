// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
#include <cassert>
#include <linalg>
int main() {
  int data[64]{};
  using E = std::extents<int, 3, 4>;
  std::mdspan<int, E, std::layout_left_padded<8>> a(data, std::layout_left_padded<8>::mapping<E>{E{}});
  auto t = std::linalg::transposed(a);
  static_assert(std::is_same_v<typename decltype(t)::layout_type, std::layout_right_padded<8>>);
  assert(t.mapping().stride(0) == 8 && t.mapping().stride(1) == 1);
}
