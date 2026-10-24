//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// [optional.comp.with.t]: an optional specialization is excluded as the
// non-optional operand of each comparison overload.

#include <optional>

template <int>
struct Right {
  Right()             = default;
  Right(const Right&) = delete;
  Right(Right&&)      = delete;
};
template <int>
struct RightValue {
  RightValue()                  = default;
  RightValue(const RightValue&) = delete;
  RightValue(RightValue&&)      = delete;
};
template <int>
struct Left {
  Left()            = default;
  Left(const Left&) = delete;
  Left(Left&&)      = delete;
};
template <int>
struct LeftValue {
  LeftValue()                 = default;
  LeftValue(const LeftValue&) = delete;
  LeftValue(LeftValue&&)      = delete;
};
bool operator==(const Right<0>&, const std::optional<RightValue<0>>&);
bool operator!=(const Right<1>&, const std::optional<RightValue<1>>&);
bool operator<(const Right<2>&, const std::optional<RightValue<2>>&);
bool operator<=(const Right<3>&, const std::optional<RightValue<3>>&);
bool operator>(const Right<4>&, const std::optional<RightValue<4>>&);
bool operator>=(const Right<5>&, const std::optional<RightValue<5>>&);
bool operator==(const std::optional<Left<0>>&, const LeftValue<0>&);
bool operator!=(const std::optional<Left<1>>&, const LeftValue<1>&);
bool operator<(const std::optional<Left<2>>&, const LeftValue<2>&);
bool operator<=(const std::optional<Left<3>>&, const LeftValue<3>&);
bool operator>(const std::optional<Left<4>>&, const LeftValue<4>&);
bool operator>=(const std::optional<Left<5>>&, const LeftValue<5>&);
#define CHECK(OP, N)                                                                                                   \
  template <class A, class B>                                                                                          \
  concept Has##N = requires(const A& a, const B& b) { a OP b; };                                                       \
  static_assert(!Has##N<std::optional<Right<N>>, std::optional<RightValue<N>>>);                                       \
  static_assert(!Has##N<std::optional<Left<N>>, std::optional<LeftValue<N>>>);                                         \
  static_assert(Has##N<std::optional<int>, std::optional<int>>);                                                       \
  static_assert(Has##N<std::optional<int>, int>);                                                                      \
  static_assert(Has##N<int, std::optional<int>>);
CHECK(==, 0)
CHECK(!=, 1)
CHECK(<, 2)
CHECK(<=, 3)
CHECK(>, 4)
CHECK(>=, 5)
#undef CHECK
