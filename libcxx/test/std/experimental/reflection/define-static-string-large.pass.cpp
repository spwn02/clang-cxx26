//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest -fconstexpr-steps=268435456

#include <experimental/meta>
#include <cstring>
#include <string>

template <std::size_t N>
consteval const char* lift() {
  return std::define_static_string(std::string(N, 'x'));
}

int main(int, char**) {
  constexpr const char* a = lift<32767>();
  if (std::strlen(a) != 32767)
    return 1;
  constexpr const char* b = lift<32768>();
  if (std::strlen(b) != 32768)
    return 2;
  constexpr const char* c = lift<50000>();
  if (std::strlen(c) != 50000)
    return 3;
  return 0;
}
