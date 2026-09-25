//===----------------------------------------------------------------------===//
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// ADDITIONAL_COMPILE_FLAGS: -ffreestanding
//===----------------------------------------------------------------------===//

#include <cstring>
#include <version>

#ifndef _LIBCPP_FREESTANDING
#  error "-ffreestanding must select libc++ freestanding mode"
#endif

#ifndef __cpp_lib_freestanding_cstring
#  error "<cstring> must advertise its freestanding subset"
#endif


// std::strtok's removal is checked separately in strtok.compile.fail.cpp:
// `std::strtok` is a qualified name, so its lookup isn't deferred by a
// template parameter and can't be tested via requires-expression SFINAE.

void test_freestanding_cstring(const char* input, char* output) {
  (void)std::memcpy(output, input, 1);
  (void)std::strcmp(input, output);
  (void)std::strlen(input);
}
