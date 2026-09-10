//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <stacktrace>

// class stacktrace_entry;
//
// constexpr stacktrace_entry() noexcept;
// constexpr native_handle_type native_handle() const noexcept;
// constexpr explicit operator bool() const noexcept;
// string description() const;
// string source_file() const;
// uint_least32_t source_line() const;

#include <cassert>
#include <stacktrace>
#include <type_traits>

#include "test_macros.h"

int main(int, char**) {
  constexpr std::stacktrace_entry e;
  static_assert(e.native_handle() == 0);
  static_assert(!static_cast<bool>(e));

  ASSERT_NOEXCEPT(std::stacktrace_entry());
  ASSERT_NOEXCEPT(e.native_handle());
  ASSERT_NOEXCEPT(static_cast<bool>(e));
  ASSERT_SAME_TYPE(decltype(e.native_handle()), std::stacktrace_entry::native_handle_type);

  // An empty entry's resolution queries are all specified to come back
  // empty/zero without needing to consult any debug info.
  assert(e.description().empty());
  assert(e.source_file().empty());
  assert(e.source_line() == 0);

  std::stacktrace_entry copy = e;
  assert(copy == e);
  std::stacktrace_entry moved = std::move(copy);
  assert(moved == e);

  return 0;
}
