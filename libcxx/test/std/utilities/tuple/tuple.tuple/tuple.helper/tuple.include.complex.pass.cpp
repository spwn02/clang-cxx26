//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <complex> must provide tuple_size/tuple_element cv specializations.

#include <complex>
#include <type_traits>

static_assert(std::tuple_size<const std::complex<float>>::value == 2);
static_assert(std::is_same_v<std::tuple_element_t<0, const std::complex<float>>, const float>);
static_assert(std::is_same_v<std::tuple_element_t<1, const std::complex<float>>, const float>);

int main(int, char**) { return 0; }
