//===----------------------------------------------------------------------===//
// UNSUPPORTED: c++03
// ADDITIONAL_COMPILE_FLAGS: -ffreestanding
//===----------------------------------------------------------------------===//

// P2976R1: freestanding <algorithm> must not provide std::inplace_merge (needs
// auxiliary heap storage this fork's freestanding mode can't guarantee).

#include <algorithm>

void f(int* first, int* middle, int* last) { std::inplace_merge(first, middle, last); }
