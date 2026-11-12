//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

// Exercise the binary16 and bfloat16 conversion fallback used when the target
// has no 128-bit integer type.
#undef __SIZEOF_INT128__

#include <charconv>

#if defined(__STDCPP_FLOAT16_T__) || defined(__STDCPP_BFLOAT16_T__)
template <class T>
bool equals(T value, const char* expected) {
  char buffer[64];
  auto result = std::to_chars(buffer, buffer + sizeof(buffer), value);
  if (result.ec != std::errc{})
    return false;
  const char* current = expected;
  for (char* output = buffer; output != result.ptr; ++output, ++current)
    if (*output != *current)
      return false;
  return *current == '\0';
}
#endif

int main(int, char**) {
#if defined(__STDCPP_FLOAT16_T__)
  if (!equals(std::float16_t(999), "999") || !equals(std::float16_t(1000), "1e+03") ||
      !equals(std::float16_t(0.5), "0.5"))
    return 1;
#endif
#if defined(__STDCPP_BFLOAT16_T__)
  if (!equals(std::bfloat16_t(99), "99") || !equals(std::bfloat16_t(100), "1e+02") ||
      !equals(std::bfloat16_t(0.5), "0.5"))
    return 1;
#endif
}
