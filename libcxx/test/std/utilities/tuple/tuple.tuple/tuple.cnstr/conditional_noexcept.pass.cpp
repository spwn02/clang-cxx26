//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03

#include <tuple>
#include <utility>

struct NothrowCopy {
  constexpr NothrowCopy() = default;
  constexpr NothrowCopy(const NothrowCopy&) noexcept = default;
};

struct ThrowingCopy {
  constexpr ThrowingCopy() = default;
  constexpr ThrowingCopy(const ThrowingCopy&) noexcept(false) {}
};

static_assert(noexcept(std::tuple<NothrowCopy>(std::declval<const NothrowCopy&>())));
static_assert(!noexcept(std::tuple<ThrowingCopy>(std::declval<const ThrowingCopy&>())));
static_assert(noexcept(std::tuple<NothrowCopy>(NothrowCopy{})));
static_assert(!noexcept(std::tuple<ThrowingCopy>(ThrowingCopy{})));
static_assert(!noexcept(std::tuple<NothrowCopy, ThrowingCopy>(
    std::declval<const NothrowCopy&>(), std::declval<const ThrowingCopy&>())));

int main(int, char**) { return 0; }
