// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
#include <cassert>
#include <mdspan>
#include <stdexcept>
int main() {
  int data[6]{};
  std::mdspan<int, std::extents<int, 2, 3>> m(data);
  m.at(1, 2) = 42;
  assert(data[5] == 42);
  bool threw = false;
  try { (void)m.at(2, 0); } catch (const std::out_of_range&) { threw = true; }
  assert(threw);
  assert(m.at(std::array<int, 2>{1, 2}) == 42);
}
