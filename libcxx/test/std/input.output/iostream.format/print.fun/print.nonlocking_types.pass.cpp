//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20
// UNSUPPORTED: no-filesystem, no-localization

// XFAIL: availability-fp_to_chars-missing

// <print>

// P3235R3 / LWG4399: pair, tuple and the chrono types enable enable_nonlocking_formatter_optimization (unless
// [time.format] specifies otherwise); print and println have to keep producing the same text as format for them.

#include <cassert>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <print>
#include <string>
#include <tuple>
#include <utility>

#include "filesystem_test_helper.h"
#include "test_macros.h"

scoped_test_env env;
std::string filename = env.create_file("output.txt");

std::string read_file() {
  std::ifstream stream{filename.c_str(), std::ios_base::in | std::ios_base::binary};
  return std::string(std::istreambuf_iterator<char>{stream}, {});
}

// print writes to the file, format produces the expected text.
#define CHECK(...)                                                                                                     \
  do {                                                                                                                 \
    FILE* file = std::fopen(filename.c_str(), "wb");                                                                   \
    assert(file);                                                                                                      \
    std::print(file, __VA_ARGS__);                                                                                     \
    std::fclose(file);                                                                                                 \
    assert(read_file() == std::format(__VA_ARGS__));                                                                   \
  } while (false)

int main(int, char**) {
  using namespace std::chrono;
  CHECK("{}", std::pair{1, 2.5});
  CHECK("{:n}", std::tuple{1, 'c', "text"});
  CHECK("{}", std::tuple<>{});
  CHECK("{}", std::make_pair(std::string("a"), 3));
  CHECK("{}", std::chrono::day{5});
  CHECK("{}", std::chrono::month{7});
  CHECK("{}", std::chrono::year{2026});
  CHECK("{}", std::chrono::weekday{3});
  CHECK("{}", std::chrono::year_month_day{2026y, October, 5d});
  CHECK("{:%F}", std::chrono::year_month_day{2026y, October, 5d});
  CHECK("{:%T}", std::chrono::hh_mm_ss{3h + 4min + 5s});
  CHECK("{:%F %T}", sys_seconds{sys_days{2026y / October / 5} + 6h});
  CHECK("{}", seconds{42});
  CHECK("{}", sys_seconds{sys_days{2026y / October / 5}});
  return 0;
}
