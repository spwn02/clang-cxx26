//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: no-threads

// <stop_token>

// class never_stop_token;

#include <cassert>
#include <stop_token>
#include <type_traits>

int main(int, char**) {
  using std::never_stop_token;

  static_assert(std::stoppable_token<never_stop_token>);
  static_assert(std::unstoppable_token<never_stop_token>);

  static_assert(never_stop_token::stop_requested() == false);
  static_assert(never_stop_token::stop_possible() == false);
  static_assert(noexcept(never_stop_token::stop_requested()));
  static_assert(noexcept(never_stop_token::stop_possible()));

  never_stop_token tok;
  assert(!tok.stop_requested());
  assert(!tok.stop_possible());
  assert(tok == never_stop_token{});

  // `callback_type<Fn>`: constructible from a token and anything, and does nothing (there's
  // never a stop request for it to react to).
  int count = 0;
  never_stop_token::callback_type<decltype([] {})> cb(tok, [&] { ++count; });
  (void)cb;
  assert(count == 0);

  return 0;
}
