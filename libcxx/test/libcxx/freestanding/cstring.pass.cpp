//===----------------------------------------------------------------------===//
// UNSUPPORTED: c++03
// RUN: %{cxx} %{flags} %{compile_flags} -ffreestanding -fsyntax-only %s
//===----------------------------------------------------------------------===//

#include <cstring>
#include <version>

#ifndef _LIBCPP_FREESTANDING
#  error "-ffreestanding must select libc++ freestanding mode"
#endif

#ifndef __cpp_lib_freestanding_cstring
#  error "<cstring> must advertise its freestanding subset"
#endif

void test_freestanding_cstring(const char* input, char* output) {
  (void)std::memcpy(output, input, 1);
  (void)std::strcmp(input, output);
  (void)std::strlen(input);
}
