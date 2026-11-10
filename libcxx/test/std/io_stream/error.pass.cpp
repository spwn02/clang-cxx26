//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// REQUIRES: std-at-least-c++29
// UNSUPPORTED: no-threads
// ADDITIONAL_COMPILE_FLAGS: -fexperimental-library

// <io_stream>

// std::io::error, std::io::cond, io_category(), io_result.

#include <io_stream>

#include <cassert>
#include <string>
#include <system_error>
#include <tuple>
#include <type_traits>

#include "test_macros.h"

namespace io = std::io;

static_assert(std::is_error_code_enum_v<io::error>);
static_assert(std::is_error_condition_enum_v<io::cond>);
static_assert(!std::is_error_condition_enum_v<io::error>);
static_assert(std::is_same_v<io::io_result<std::size_t>, std::tuple<std::error_code, std::size_t>>);
static_assert(std::is_same_v<io::io_result<>, std::tuple<std::error_code>>);

int main(int, char**) {
  std::error_code ec = io::error::eof;
  assert(ec);
  assert(ec.category() == io::io_category());
  assert(ec == io::error::eof);
  assert(ec != io::error::canceled);
  // Received codes are compared against the portable conditions.
  assert(ec == io::cond::eof);
  assert(ec != io::cond::canceled);
  assert(ec.default_error_condition() == io::cond::eof);
  assert(ec.default_error_condition().category() == io::io_condition_category());
  assert(io::io_category() != io::io_condition_category());
  assert(std::string(io::io_category().name()) == "std::io");

  for (io::error e : {io::error::eof, io::error::canceled, io::error::stream_truncated, io::error::timeout}) {
    std::error_code c = e;
    assert(!c.message().empty() && c.message().find("unknown") == std::string::npos);
    assert(c == io::cond(static_cast<int>(e)));
    assert(c != std::error_code{});
  }
  assert((std::error_code(io::error::timeout) == io::cond::timeout));
  assert((std::error_code(io::error::stream_truncated) == io::cond::stream_truncated));

  // Platform codes that mean the same thing match the portable conditions.
  assert(std::make_error_code(std::errc::operation_canceled) == io::cond::canceled);
  assert(std::make_error_code(std::errc::timed_out) == io::cond::timeout);
  assert(std::make_error_code(std::errc::operation_canceled) != io::cond::timeout);
  assert(std::make_error_code(std::errc::operation_canceled) != io::cond::eof);

  // An io error does not compare equal to a different system error.
  assert(ec != std::errc::connection_reset);
  assert(std::make_error_code(std::errc::connection_reset) != io::cond::eof);

  // Structured bindings and std::tie work on results.
  io::io_result<std::size_t> r{io::make_error_code(io::error::canceled), 7};
  auto [e, n] = r;
  assert(e == io::cond::canceled && n == 7);
  std::error_code e2;
  std::size_t n2;
  std::tie(e2, n2) = r;
  assert(e2 == e && n2 == 7);
  io::io_result<> r0{};
  auto [e0] = r0;
  assert(!e0);

  return 0;
}
