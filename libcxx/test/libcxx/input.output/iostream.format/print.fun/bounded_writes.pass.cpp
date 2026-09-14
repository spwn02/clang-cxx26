//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

// Verify the P3107R5 implementation mechanism, not only its output.

#include <cassert>
#include <chrono>
#include <ostream>
#include <print>
#include <string>
#include <tuple>
#include <vector>

struct fallback_type {
  int value;
};

template <>
struct std::formatter<fallback_type, char> : std::formatter<int, char> {
  auto format(const fallback_type& __value, auto& __ctx) const {
    return std::formatter<int, char>::format(__value.value, __ctx);
  }
};

struct recording_buf : std::streambuf {
  std::vector<std::size_t> writes;
  std::string output;
  std::size_t fail_after = static_cast<std::size_t>(-1);

protected:
  std::streamsize xsputn(const char* __data, std::streamsize __size) override {
    writes.push_back(static_cast<std::size_t>(__size));
    if (writes.size() > fail_after)
      return 0;
    output.append(__data, static_cast<std::size_t>(__size));
    return __size;
  }
};

int main(int, char**) {
  static_assert(std::enable_nonlocking_formatter_optimization<std::tuple<int, std::string>>);
  static_assert(std::enable_nonlocking_formatter_optimization<std::chrono::sys_days>);
  static_assert(std::enable_nonlocking_formatter_optimization<std::chrono::duration<int>>);

  std::string expected(4097, 'x');
  recording_buf __buf;
  std::ostream __os(&__buf);

  std::__vprint_nonunicode(__os, "{}", std::make_format_args(expected), false);
  assert(__buf.output == expected);
  assert(__buf.writes.size() > 1);
  for (std::size_t __size : __buf.writes)
    assert(__size <= 1024);

  __buf.output.clear();
  __buf.writes.clear();
  __buf.fail_after = 1;
  __os.exceptions(std::ios_base::badbit | std::ios_base::failbit);
  bool __threw = false;
  try {
    std::__vprint_nonunicode(__os, "{}", std::make_format_args(expected), false);
  } catch (const std::ios_base::failure&) {
    __threw = true;
  }
  assert(__threw);
  assert(__buf.writes.size() >= 2);
  assert(__buf.output.size() < expected.size());

  __buf.output.clear();
  __buf.writes.clear();
  __buf.fail_after = static_cast<std::size_t>(-1);
  __os.clear();
  std::print(__os, "{}", fallback_type{42});
  assert(__buf.output == "42");
  assert(__buf.writes.size() == 1);
  return 0;
}
