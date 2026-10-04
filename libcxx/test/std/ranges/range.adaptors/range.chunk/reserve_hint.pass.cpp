//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

#include "../reserve_hint_test.h"

// clang-format off
// Adopted draft: chunk_view reserve_hint (raw clause text).
// constexpr auto reserve_hint() requires @\libconcept{approximately_sized_range}@<V>;
// constexpr auto reserve_hint() const requires @\libconcept{approximately_sized_range}@<const V>;
// auto s = static_cast<range_difference_t<decltype((@\exposidnc{base_}@))>>(ranges::reserve_hint(@\exposidnc{base_}@));
// return @\exposidnc{to-unsigned-like}@(@\exposidnc{div-ceil}@(s, @\exposidnc{n_}@));
// constexpr auto reserve_hint() requires @\libconcept{approximately_sized_range}@<V>;
// constexpr auto reserve_hint() const requires @\libconcept{approximately_sized_range}@<const V>;
// auto s = static_cast<range_difference_t<decltype((@\exposid{base_}@))>>(ranges::reserve_hint(@\exposid{base_}@));
// return @\exposid{to-unsigned-like}@(@\exposid{div-ceil}@(s, @\exposid{n_}@));
// constexpr auto reserve_hint() const noexcept;
// return @\exposid{to-unsigned-like}@(@\exposid{parent_}@->@\exposid{remainder_}@);
// clang-format on
template <bool Sized>
constexpr bool test() {
  using Value = int;
  using Base  = HintView<Value, Sized>;
  Value storage[5]{};
  for (int length : {0, 1, 5}) {
    for (int estimate : {0, 1, 8}) {
      Base base(storage, length, estimate);
      [[maybe_unused]] int hint = Sized ? length : estimate;
      for ([[maybe_unused]] int count : {1, 2, 10}) {
        std::ranges::chunk_view value(base, count);
        check_hint(value, hint / count + (hint % count != 0));
        static_assert(std::same_as<decltype(value.reserve_hint()), std::make_unsigned_t<std::ptrdiff_t>>);
      }
    }
  }
  return true;
}

using MutableHintBase = HintView<int, false, false>;
using NoHintBase      = HintView<int, false, true, false>;
using MutableHintView = std::ranges::chunk_view<MutableHintBase>;
using NoHintView      = std::ranges::chunk_view<NoHintBase>;
static_assert(HasReserveHint<MutableHintView>);
static_assert(!HasReserveHint<const MutableHintView>);
static_assert(!HasReserveHint<NoHintView>);
static_assert(!HasReserveHint<const NoHintView>);

// [range.chunk.view], [range.chunk.outer.value]: same outer hint formula for
// input-only bases; the inner hint uses the remaining count, even when unsized.
template <bool Sized>
constexpr bool test_input() {
  int storage[5]{};
  using Base = HintView<int, Sized, true, true, true>;
  static_assert(!std::ranges::forward_range<Base>);
  std::ranges::chunk_view value(Base(storage, 5, 8), 3);
  check_hint(value, Sized ? 2 : 3);
  auto outer = value.begin();
  auto inner = *outer;
  static_assert(noexcept(std::as_const(inner).reserve_hint()));
  assert(inner.reserve_hint() == 3);
  auto position = inner.begin();
  ++position;
  assert(inner.reserve_hint() == 2);
  ++position;
  assert(inner.reserve_hint() == 1);
  ++position;
  assert(inner.reserve_hint() == 0);
  ++outer;
  auto last = *outer;
  assert(last.reserve_hint() == 3);
  auto last_position = last.begin();
  ++last_position;
  assert(last.reserve_hint() == 2);
  ++last_position;
  assert(last.reserve_hint() == 0);
  return true;
}
static_assert(test_input<false>());
static_assert(test_input<true>());

int main(int, char**) {
  test<false>();
  test<true>();
  static_assert(test<false>());
  static_assert(test<true>());
  test_input<false>();
  test_input<true>();
  return 0;
}
