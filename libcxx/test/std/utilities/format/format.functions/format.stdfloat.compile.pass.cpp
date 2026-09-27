// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++17, c++20

#include <format>
#include <stdfloat>

static_assert(std::formattable<std::float16_t, char>);
static_assert(std::formattable<std::float32_t, char>);
static_assert(std::formattable<std::float64_t, char>);
static_assert(std::formattable<std::bfloat16_t, char>);

int main(int, char**) {
  (void)std::format("{} {:f} {:e} {:g} {:a}", 1.5f16, 1.5f32, 1.5f64, 1.5bf16, 1.5f16);
}
