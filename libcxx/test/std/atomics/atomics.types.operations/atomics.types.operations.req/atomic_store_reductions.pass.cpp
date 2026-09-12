// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
#include <atomic>
#include <cassert>
int main(int, char**) {
  std::atomic<int> a(1);
  std::atomic_store_add(&a, 2); std::atomic_store_sub(&a, 1); std::atomic_store_and(&a, 3);
  std::atomic_store_or(&a, 4); std::atomic_store_xor(&a, 1); std::atomic_store_max(&a, 9); std::atomic_store_min(&a, 3);
  assert(a.load() == 3);
}
