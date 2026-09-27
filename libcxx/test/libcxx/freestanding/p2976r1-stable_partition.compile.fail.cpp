//===----------------------------------------------------------------------===//
// UNSUPPORTED: c++03
// ADDITIONAL_COMPILE_FLAGS: -ffreestanding
//===----------------------------------------------------------------------===//

// P2976R1: freestanding <algorithm> must not provide std::stable_partition (needs
// auxiliary heap storage this fork's freestanding mode can't guarantee).

#include <algorithm>

void f(int* first, int* last) {
  std::stable_partition(first, last, [](int value) { return value != 0; });
}
