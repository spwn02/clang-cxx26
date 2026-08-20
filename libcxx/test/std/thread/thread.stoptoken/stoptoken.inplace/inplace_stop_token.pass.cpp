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

// class inplace_stop_token;

#include <cassert>
#include <concepts>
#include <stop_token>
#include <type_traits>

static_assert(std::is_trivially_copyable_v<std::inplace_stop_token>);
static_assert(std::semiregular<std::inplace_stop_token>);

int main(int, char**) {
  using std::inplace_stop_source;
  using std::inplace_stop_token;

  {
    // default-constructed token is not associated with any source
    constexpr inplace_stop_token tok;
    static_assert(!tok.stop_possible());
    static_assert(tok == inplace_stop_token{});
  }

  {
    inplace_stop_source src;
    inplace_stop_token tok = src.get_token();
    assert(tok.stop_possible());
    assert(!tok.stop_requested());

    // copies refer to the same source
    inplace_stop_token copy = tok;
    assert(copy == tok);

    src.request_stop();
    assert(tok.stop_requested());
    assert(copy.stop_requested());
  }

  {
    // swap
    inplace_stop_source src1;
    inplace_stop_source src2;
    inplace_stop_token tok1 = src1.get_token();
    inplace_stop_token tok2 = src2.get_token();

    src1.request_stop();
    assert(tok1.stop_requested());
    assert(!tok2.stop_requested());

    swap(tok1, tok2);
    assert(!tok1.stop_requested());
    assert(tok2.stop_requested());
  }

  return 0;
}
