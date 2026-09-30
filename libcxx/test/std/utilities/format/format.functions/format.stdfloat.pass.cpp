// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

#include <format>
#include <stdfloat>
#include <cassert>
#include <string>

static_assert(std::formattable<std::float16_t, char>);
static_assert(std::formattable<std::float32_t, char>);
static_assert(std::formattable<std::float64_t, char>);
static_assert(std::formattable<std::bfloat16_t, char>);

int main(int, char**) {
  assert(std::format("{}", 1.5f16) == "1.5");
  assert(std::format("{:+08.2f}", 1.5bf16) == "+0001.50");
  assert(std::format("{:^8.3}", 1.5f32) == "  1.5   ");
  assert(std::format("{:e}", 1.5f64) == "1.500000e+00"); // {:e} defaults to precision 6, same as double
  assert(std::format("{:g}", 1.5f16) == "1.5");
  assert(std::format("{:a}", 1.5bf16) == "1.8p+0"); // matches float/double's own {:a} (no '0x' without '#')

  std::string output;
  std::format_to(std::back_inserter(output), "{:.3f}", 1.5f16);
  assert(output == "1.500");
  std::float32_t value = 1.5f32;
  assert(std::vformat("{:>7.2f}", std::make_format_args(value)) == "   1.50");
  assert(std::format("{} {}", 1.5f64, 1.5bf16) == "1.5 1.5");
}
