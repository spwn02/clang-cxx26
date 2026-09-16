//===----------------------------------------------------------------------===//
// UNSUPPORTED: c++03
// ADDITIONAL_COMPILE_FLAGS: -ffreestanding
//===----------------------------------------------------------------------===//

// P2937R0: freestanding <cstring> must not provide std::strtok.

#include <cstring>

void f(char* s, const char* delim) { std::strtok(s, delim); }
