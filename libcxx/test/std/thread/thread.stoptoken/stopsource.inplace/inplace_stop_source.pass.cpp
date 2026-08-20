//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: no-threads
// XFAIL: availability-synchronization_library-missing

// <stop_token>

// class inplace_stop_source;

#include <cassert>
#include <stop_token>
#include <type_traits>
#include <utility>

static_assert(!std::is_copy_constructible_v<std::inplace_stop_source>);
static_assert(!std::is_move_constructible_v<std::inplace_stop_source>);
static_assert(!std::is_copy_assignable_v<std::inplace_stop_source>);
static_assert(!std::is_move_assignable_v<std::inplace_stop_source>);

int main(int, char**) {
  using std::inplace_stop_source;

  static_assert(inplace_stop_source::stop_possible());
  static_assert(noexcept(std::declval<inplace_stop_source&>().stop_possible()));

  {
    inplace_stop_source src;
    assert(!src.stop_requested());

    auto tok = src.get_token();
    static_assert(std::is_same_v<decltype(tok), std::inplace_stop_token>);
    assert(tok.stop_possible());
    assert(!tok.stop_requested());

    assert(src.request_stop());
    assert(src.stop_requested());
    assert(tok.stop_requested());

    // a second request_stop() is a no-op that reports it did nothing new
    assert(!src.request_stop());
  }

  return 0;
}
