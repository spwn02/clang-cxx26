//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef RESERVE_HINT_TEST_H
#define RESERVE_HINT_TEST_H

#include <ranges>
#include <cassert>
#include <cstddef>
#include <initializer_list>
#include <tuple>
#include <type_traits>
#include <utility>
#include "test_iterators.h"

template <class Value = int, bool Sized = false, bool ConstHint = true, bool HasHint = true, bool Input = false>
struct HintView : std::ranges::view_interface<HintView<Value, Sized, ConstHint, HasHint, Input>> {
  Value* first          = nullptr;
  std::ptrdiff_t length = 0;
  int hint              = 0;
  using Iterator        = std::conditional_t<Input, cpp20_input_iterator<Value*>, Value*>;
  constexpr HintView()  = default;
  constexpr HintView(Value* p, std::ptrdiff_t n, int h) : first(p), length(n), hint(h) {}
  constexpr Iterator begin() const { return Iterator(first); }
  constexpr auto end() const { return sentinel_wrapper<Iterator>(Iterator(first + length)); }
  constexpr unsigned size() const
    requires Sized
  {
    return static_cast<unsigned>(length);
  }
  constexpr int reserve_hint()
    requires HasHint
  {
    return hint;
  }
  constexpr int reserve_hint() const
    requires(HasHint && ConstHint)
  {
    return hint;
  }
};

template <class View>
concept HasReserveHint = requires(View& value) { value.reserve_hint(); };

struct Identity {
  constexpr int operator()(int value) const { return value; }
};
struct Add {
  constexpr int operator()(int left, int right) const { return left + right; }
};

template <class View, class Result>
constexpr void check_hint(View& value, Result expected) {
  assert(value.reserve_hint() == static_cast<decltype(value.reserve_hint())>(expected));
  assert(std::as_const(value).reserve_hint() == static_cast<decltype(value.reserve_hint())>(expected));
  static_assert(std::same_as<decltype(value.reserve_hint()), decltype(std::as_const(value).reserve_hint())>);
}

static_assert(std::ranges::sized_range<HintView<int, true>>);
static_assert(!std::ranges::sized_range<HintView<>>);
static_assert(std::ranges::approximately_sized_range<HintView<>>);
static_assert(std::ranges::approximately_sized_range<const HintView<>>);
static_assert(!std::ranges::approximately_sized_range<const HintView<int, false, false>>);
static_assert(!std::ranges::approximately_sized_range<HintView<int, false, true, false>>);
#endif
