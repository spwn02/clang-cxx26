//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// std::ranges::reserve_hint

#include <ranges>

#include <cassert>
#include <type_traits>
#include <utility>
#include "test_macros.h"

using RangeReserveHintT = decltype(std::ranges::reserve_hint);

// [range.prim.size.hint]/1: if `ranges::size(E)` is valid, `reserve_hint` is
// expression-equivalent to `ranges::size(E)` -- takes priority over any
// member/ADL `reserve_hint`.
struct SizeMember {
  constexpr std::size_t size() const { return 42; }
  constexpr std::size_t reserve_hint() const { return 0; } // should be ignored: size() wins
};
static_assert(std::is_invocable_v<RangeReserveHintT, SizeMember&>);

constexpr bool testSizeWins() {
  SizeMember s;
  assert(std::ranges::reserve_hint(s) == 42);
  ASSERT_SAME_TYPE(decltype(std::ranges::reserve_hint(s)), std::size_t);
  return true;
}

// [range.prim.size.hint]/2: no `size()`, has a member `reserve_hint()`.
struct ReserveHintMember {
  constexpr std::size_t reserve_hint() const { return 7; }
};
static_assert(std::is_invocable_v<RangeReserveHintT, ReserveHintMember&>);

constexpr bool testMember() {
  ReserveHintMember r;
  assert(std::ranges::reserve_hint(r) == 7);
  return true;
}

struct ReserveHintMemberSigned {
  constexpr long reserve_hint() const { return 9; }
};
constexpr bool testMemberSigned() {
  ReserveHintMemberSigned r;
  assert(std::ranges::reserve_hint(r) == 9);
  ASSERT_SAME_TYPE(decltype(std::ranges::reserve_hint(r)), long);
  return true;
}

// [range.prim.size.hint]/3: no `size()`, no member, ADL `reserve_hint(t)`.
struct ReserveHintFunction {
  friend constexpr std::size_t reserve_hint(ReserveHintFunction) { return 11; }
};
static_assert(std::is_invocable_v<RangeReserveHintT, ReserveHintFunction&>);

constexpr bool testADL() {
  ReserveHintFunction r;
  assert(std::ranges::reserve_hint(r) == 11);
  return true;
}

// Member is preferred over ADL when both are present.
struct ReserveHintMemberAndFunction {
  constexpr std::size_t reserve_hint() const { return 42; }
  friend constexpr std::size_t reserve_hint(ReserveHintMemberAndFunction) { return 0; }
};
constexpr bool testMemberOverADL() {
  ReserveHintMemberAndFunction r;
  assert(std::ranges::reserve_hint(r) == 42);
  return true;
}

// Neither size, nor member, nor ADL -- not invocable.
struct Empty {};
static_assert(!std::is_invocable_v<RangeReserveHintT, Empty&>);

// A return type that isn't integer-like disqualifies the member/ADL overloads
// (but note the array/`size()` overload always short-circuits first).
struct NotIntegerLike {};
struct InvalidMember {
  NotIntegerLike reserve_hint() const;
};
static_assert(!std::is_invocable_v<RangeReserveHintT, InvalidMember&>);

struct InvalidFunction {
  friend NotIntegerLike reserve_hint(InvalidFunction);
};
static_assert(!std::is_invocable_v<RangeReserveHintT, InvalidFunction&>);

// Arrays go through the `size()` overload.
static_assert(std::is_invocable_v<RangeReserveHintT, int (&)[4]>);
constexpr bool testArray() {
  int a[4];
  assert(std::ranges::reserve_hint(a) == 4);
  return true;
}

// noexcept propagates from the selected path.
struct ThrowingSize {
  std::size_t size() const noexcept(false) { return 1; }
};
static_assert(!noexcept(std::ranges::reserve_hint(std::declval<ThrowingSize&>())));

struct NoexceptSize {
  constexpr std::size_t size() const noexcept { return 1; }
};
static_assert(noexcept(std::ranges::reserve_hint(std::declval<NoexceptSize&>())));

int main(int, char**) {
  testSizeWins();
  static_assert(testSizeWins());

  testMember();
  static_assert(testMember());

  testMemberSigned();
  static_assert(testMemberSigned());

  testADL();
  static_assert(testADL());

  testMemberOverADL();
  static_assert(testMemberOverADL());

  testArray();
  static_assert(testArray());

  return 0;
}
