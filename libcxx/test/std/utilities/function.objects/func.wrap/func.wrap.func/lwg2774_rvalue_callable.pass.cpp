//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03

#include <functional>
#include <utility>
#include "test_macros.h"

struct Callable {
    int* moves;

    explicit Callable(int& count) : moves(&count) {}
    Callable(const Callable&) = default;
    Callable(Callable&& other) noexcept : moves(other.moves) { ++*moves; }
    int operator()() const { return 0; }
};

int main() {
    int moves = 0;
    Callable callable(moves);
    std::function<int()> wrapped(std::move(callable));
    return moves == 1 && wrapped() == 0 ? 0 : 1;
}
