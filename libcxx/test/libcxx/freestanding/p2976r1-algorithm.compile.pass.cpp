//===----------------------------------------------------------------------===//
// UNSUPPORTED: c++03
// ADDITIONAL_COMPILE_FLAGS: -ffreestanding
//===----------------------------------------------------------------------===//

// P2976R1: freestanding <algorithm> -- every algorithm remains available under
// -ffreestanding except stable_sort/stable_partition/inplace_merge (see the sibling
// p2976r1-stable_sort/stable_partition/inplace_merge.compile.fail.cpp negative tests --
// those three need auxiliary heap storage this fork's freestanding mode can't guarantee).

#include <algorithm>

void test_freestanding_algorithm_surface(int* first, int* last, int* middle) {
  std::sort(first, last);
  (void)std::find(first, last, 2);
  std::rotate(first, middle, last);
  std::partition(first, last, [](int v) { return v != 0; });
  (void)std::min_element(first, last);
  std::nth_element(first, middle, last);
  std::push_heap(first, last);
  std::make_heap(first, last);
}
