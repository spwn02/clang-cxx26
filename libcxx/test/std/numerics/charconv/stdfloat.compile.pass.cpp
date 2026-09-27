// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++17, c++20

#include <charconv>
#include <stdfloat>
#include <cassert>

int main(int, char**) {
  char buffer[128];
#if defined(__STDCPP_FLOAT16_T__)
  std::float16_t half{};
  (void)std::to_chars(buffer, buffer + sizeof(buffer), half);
  (void)std::to_chars(buffer, buffer + sizeof(buffer), half, std::chars_format::general);
  (void)std::to_chars(buffer, buffer + sizeof(buffer), half, std::chars_format::fixed, 3);
  (void)std::from_chars(buffer, buffer + sizeof(buffer), half);
#endif
#if defined(__STDCPP_BFLOAT16_T__)
  std::bfloat16_t bhalf{};
  (void)std::to_chars(buffer, buffer + sizeof(buffer), bhalf);
  (void)std::to_chars(buffer, buffer + sizeof(buffer), bhalf, std::chars_format::general);
  (void)std::to_chars(buffer, buffer + sizeof(buffer), bhalf, std::chars_format::fixed, 3);
  (void)std::from_chars(buffer, buffer + sizeof(buffer), bhalf);
#endif
#if defined(__STDCPP_FLOAT32_T__)
  std::float32_t single{};
  (void)std::to_chars(buffer, buffer + sizeof(buffer), single);
  (void)std::to_chars(buffer, buffer + sizeof(buffer), single, std::chars_format::general);
  (void)std::to_chars(buffer, buffer + sizeof(buffer), single, std::chars_format::fixed, 3);
  (void)std::from_chars(buffer, buffer + sizeof(buffer), single);
#endif
#if defined(__STDCPP_FLOAT64_T__)
  std::float64_t dbl{};
  (void)std::to_chars(buffer, buffer + sizeof(buffer), dbl);
  (void)std::to_chars(buffer, buffer + sizeof(buffer), dbl, std::chars_format::general);
  (void)std::to_chars(buffer, buffer + sizeof(buffer), dbl, std::chars_format::fixed, 3);
  (void)std::from_chars(buffer, buffer + sizeof(buffer), dbl);
#endif
}
