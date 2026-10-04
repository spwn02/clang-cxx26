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
// Adopted draft: take_view reserve_hint (raw clause text).
//     constexpr auto reserve_hint() {
//       if constexpr (@\libconcept{approximately_sized_range}@<V>) {
//         auto n = static_cast<range_difference_t<V>>(ranges::reserve_hint(@\exposid{base_}@));
//         return @\exposid{to-unsigned-like}@(ranges::min(n, @\exposid{count_}@));
//       }
//       return @\exposid{to-unsigned-like}@(@\exposid{count_}@);
//     }
//
//     constexpr auto reserve_hint() const {
//       if constexpr (@\libconcept{approximately_sized_range}@<const V>) {
//         auto n = static_cast<range_difference_t<const V>>(ranges::reserve_hint(@\exposid{base_}@));
//         return @\exposid{to-unsigned-like}@(ranges::min(n, @\exposid{count_}@));
//       }
//       return @\exposid{to-unsigned-like}@(@\exposid{count_}@);
//     }
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
        std::ranges::take_view value(base, count);
        check_hint(value, hint < count ? hint : count);
        static_assert(std::same_as<decltype(value.reserve_hint()), std::make_unsigned_t<std::ptrdiff_t>>);
      }
    }
  }
  return true;
}

using MutableHintBase = HintView<int, false, false>;
using NoHintBase      = HintView<int, false, true, false>;
using MutableHintView = std::ranges::take_view<MutableHintBase>;
using NoHintView      = std::ranges::take_view<NoHintBase>;
static_assert(HasReserveHint<MutableHintView>);
static_assert(HasReserveHint<const MutableHintView>);
static_assert(HasReserveHint<NoHintView>);
static_assert(HasReserveHint<const NoHintView>);

constexpr bool test_without_hint() {
  int storage[3]{};
  NoHintView value(NoHintBase(storage, 3, 0), 7);
  check_hint(value, 7);
  return true;
}
static_assert(test_without_hint());

int main(int, char**) {
  test<false>();
  test<true>();
  static_assert(test<false>());
  static_assert(test<true>());
  test_without_hint();
  return 0;
}
