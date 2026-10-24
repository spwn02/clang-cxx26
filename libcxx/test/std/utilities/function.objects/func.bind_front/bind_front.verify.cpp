//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17

// functional

// template <class F, class... Args>
// constexpr unspecified bind_front(F&&, Args&&...);

#include <functional>

#include "test_macros.h"

constexpr int pass(const int n) { return n; }

int simple(int n) { return n; }

template<class T>
T do_nothing(T t) { return t; }

struct NotMoveConst
{
    NotMoveConst(NotMoveConst &&) = delete;
    NotMoveConst(NotMoveConst const&) = delete;

    NotMoveConst(int) { }
};

void testNotMoveConst(NotMoveConst) { }

void f() {
    int n = 1;
    const int c = 1;

    auto p = std::bind_front(pass, c);
    static_assert(p() == 1); // expected-error {{static assertion expression is not an integral constant expression}}

    auto d = std::bind_front(do_nothing, n); // expected-error {{no matching function for call to 'bind_front'}}

#if TEST_STD_VER < 26
    auto t = std::bind_front(testNotMoveConst, NotMoveConst(0)); // expected-error {{no matching function for call to 'bind_front'}}
    // expected-error@*:* 0-1{{call to deleted constructor of 'NotMoveConst'}}
#else
    // [func.bind.partial] Mandates: is_constructible_v<BoundArgs, Args> && is_move_constructible_v<BoundArgs>
    auto t = std::bind_front(testNotMoveConst, NotMoveConst(0));
    // expected-error@*:* {{bind_front requires constructible, move-constructible bound arguments}}
    // expected-error@*:* 0+ {{no matching constructor for initialization}}
    // expected-error@*:* 0+ {{call to deleted constructor of 'NotMoveConst'}}
#endif
}
