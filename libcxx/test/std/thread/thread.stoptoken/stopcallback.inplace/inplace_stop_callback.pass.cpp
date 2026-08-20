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

// template<class Callback>
//   class inplace_stop_callback;

#include <cassert>
#include <stop_token>
#include <type_traits>
#include <utility>

template <class Fn>
using CB = std::inplace_stop_callback<Fn>;

static_assert(!std::is_copy_constructible_v<CB<decltype([] {})>>);
static_assert(!std::is_move_constructible_v<CB<decltype([] {})>>);
static_assert(!std::is_copy_assignable_v<CB<decltype([] {})>>);
static_assert(!std::is_move_assignable_v<CB<decltype([] {})>>);

int main(int, char**) {
  using std::inplace_stop_callback;
  using std::inplace_stop_source;
  using std::inplace_stop_token;

  {
    // registering before a stop request: the callback fires exactly once, when requested
    inplace_stop_source src;
    int count = 0;
    auto fn = [&] { ++count; };
    inplace_stop_callback cb(src.get_token(), fn);
    static_assert(std::is_same_v<decltype(cb), inplace_stop_callback<decltype(fn)>>);
    assert(count == 0);

    assert(src.request_stop());
    assert(count == 1);

    // a second request_stop() does not re-invoke already-fired callbacks
    assert(!src.request_stop());
    assert(count == 1);
  }

  {
    // registering after a stop request already happened: fires synchronously, immediately,
    // on the constructing thread
    inplace_stop_source src;
    src.request_stop();

    int count = 0;
    inplace_stop_callback cb(src.get_token(), [&] { ++count; });
    assert(count == 1);
  }

  {
    // a token with no associated source: the callback registers cleanly but never fires
    inplace_stop_token tok;
    int count = 0;
    inplace_stop_callback cb(tok, [&] { ++count; });
    assert(count == 0);
  }

  {
    // destroying an un-fired callback deregisters it cleanly (no crash, no invocation)
    inplace_stop_source src;
    int count = 0;
    {
      inplace_stop_callback cb(src.get_token(), [&] { ++count; });
      (void)cb;
    }
    src.request_stop();
    assert(count == 0);
  }

  {
    // multiple callbacks on the same source all fire
    inplace_stop_source src;
    int count = 0;
    inplace_stop_callback cb1(src.get_token(), [&] { ++count; });
    inplace_stop_callback cb2(src.get_token(), [&] { ++count; });
    inplace_stop_callback cb3(src.get_token(), [&] { ++count; });
    src.request_stop();
    assert(count == 3);
  }

  {
    // CTAD
    inplace_stop_source src;
    auto fn = [] {};
    inplace_stop_callback cb(src.get_token(), fn);
    static_assert(std::is_same_v<decltype(cb), inplace_stop_callback<decltype(fn)>>);
  }

  return 0;
}
