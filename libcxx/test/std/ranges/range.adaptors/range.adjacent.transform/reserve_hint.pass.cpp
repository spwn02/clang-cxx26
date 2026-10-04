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
// Adopted draft: adjacent_transform_view reserve_hint (raw clause text).
//     constexpr auto reserve_hint() requires @\libconcept{approximately_sized_range}@<@\exposid{InnerView}@> {
//       return @\exposid{inner_}@.reserve_hint();
//     }
//
//     constexpr auto reserve_hint() const requires @\libconcept{approximately_sized_range}@<const @\exposid{InnerView}@> {
//       return @\exposid{inner_}@.reserve_hint();
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
        std::ranges::adjacent_transform_view<Base, Add, 2> value(base, Add{});
        check_hint(value, hint < 1 ? 0 : hint - 1);
        static_assert(std::same_as<decltype(value.reserve_hint()),
                                   std::make_unsigned_t<std::common_type_t<std::ptrdiff_t, std::size_t>>>);
      }
    }
  }
  return true;
}

using MutableHintBase = HintView<int, false, false>;
using NoHintBase      = HintView<int, false, true, false>;
using MutableHintView = std::ranges::adjacent_transform_view<MutableHintBase, Add, 2>;
using NoHintView      = std::ranges::adjacent_transform_view<NoHintBase, Add, 2>;
static_assert(HasReserveHint<MutableHintView>);
static_assert(!HasReserveHint<const MutableHintView>);
static_assert(!HasReserveHint<NoHintView>);
static_assert(!HasReserveHint<const NoHintView>);

int main(int, char**) {
  test<false>();
  test<true>();
  static_assert(test<false>());
  static_assert(test<true>());
  return 0;
}
