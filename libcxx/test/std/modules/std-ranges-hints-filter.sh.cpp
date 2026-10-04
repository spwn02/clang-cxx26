//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: clang-modules-build
// UNSUPPORTED: gcc

// XFAIL: has-no-cxx-module-support

// RUN: mkdir %t
// RUN: %{cxx} %{compile_flags} -std=c++26 -freflection-latest \
// RUN:     -Wno-reserved-module-identifier -Wno-reserved-user-defined-literal \
// RUN:     --precompile -o %t/std.pcm -c %{module-dir}/std.cppm
// RUN: %{cxx} %{compile_flags} %{link_flags} -std=c++26 -freflection-latest \
// RUN:     -fmodule-file=std=%t/std.pcm %t/std.pcm \
// RUN:     %s -o %t/std-ranges-hints-filter.sh.cpp.tsk
// RUN: %{exec} %t/std-ranges-hints-filter.sh.cpp.tsk

import std;

template <class Value>
struct HintSentinel {
  Value* last;
  friend constexpr bool operator==(Value* current, HintSentinel sent) { return current == sent.last; }
};
template <class Value, bool Sized>
struct ModuleHintView : std::ranges::view_interface<ModuleHintView<Value, Sized>> {
  Value* first;
  constexpr Value* begin() const { return first; }
  constexpr HintSentinel<Value> end() const { return {first + 5}; }
  constexpr unsigned size() const
    requires Sized
  {
    return 5;
  }
  constexpr int reserve_hint() const { return 8; }
};
struct Identity {
  constexpr int operator()(int value) const { return value; }
};
struct Add {
  constexpr int operator()(int left, int right) const { return left + right; }
};
template <class View>
constexpr bool check_hint(View& value, int expected) {
  return value.reserve_hint() == static_cast<decltype(value.reserve_hint())>(expected) &&
         std::as_const(value).reserve_hint() == static_cast<decltype(value.reserve_hint())>(expected);
}
template <bool Sized>
constexpr bool test_hints() {
  int storage[5]{};
  std::tuple<int> tuples[5]{};
  [[maybe_unused]] int hint  = Sized ? 5 : 8;
  [[maybe_unused]] int count = 2;
  {
    using Base = ModuleHintView<int, Sized>;
    Base base{{}, storage};
    // clang-format off
// Adopted draft: ref_view reserve_hint (raw clause text).
//     constexpr auto reserve_hint() const requires @\libconcept{approximately_sized_range}@<R>
//     { return ranges::reserve_hint(*@\exposid{r_}@); }
    // clang-format on
    std::ranges::ref_view value(base);
    if (!check_hint(value, hint))
      return false;
  }
  {
    using Base = ModuleHintView<int, Sized>;
    Base base{{}, storage};
    // clang-format off
// Adopted draft: owning_view reserve_hint (raw clause text).
//     constexpr auto reserve_hint() requires @\libconcept{approximately_sized_range}@<R>
//     { return ranges::reserve_hint(@\exposid{r_}@); }
//     constexpr auto reserve_hint() const requires @\libconcept{approximately_sized_range}@<const R>
//     { return ranges::reserve_hint(@\exposid{r_}@); }
    // clang-format on
    std::ranges::owning_view value(std::move(base));
    if (!check_hint(value, hint))
      return false;
  }
  {
    using Base = ModuleHintView<int, Sized>;
    Base base{{}, storage};
    // clang-format off
// Adopted draft: transform_view reserve_hint (raw clause text).
//     constexpr auto reserve_hint() requires @\libconcept{approximately_sized_range}@<V>
//     { return ranges::reserve_hint(@\exposid{base_}@); }
//     constexpr auto reserve_hint() const requires @\libconcept{approximately_sized_range}@<const V>
//     { return ranges::reserve_hint(@\exposid{base_}@); }
    // clang-format on
    std::ranges::transform_view value(base, Identity{});
    if (!check_hint(value, hint))
      return false;
  }
  {
    using Base = ModuleHintView<int, Sized>;
    Base base{{}, storage};
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
    std::ranges::take_view value(base, count);
    if (!check_hint(value, hint < count ? hint : count))
      return false;
  }
  {
    using Base = ModuleHintView<int, Sized>;
    Base base{{}, storage};
    // clang-format off
// Adopted draft: drop_view reserve_hint (raw clause text).
//     constexpr auto reserve_hint() requires @\libconcept{approximately_sized_range}@<V> {
//       const auto s = static_cast<range_difference_t<V>>(ranges::reserve_hint(@\exposid{base_}@));
//       return @\exposid{to-unsigned-like}@(s < @\exposid{count_}@ ? 0 : s - @\exposid{count_}@);
//     }
//
//     constexpr auto reserve_hint() const requires @\libconcept{approximately_sized_range}@<const V> {
//       const auto s = static_cast<range_difference_t<const V>>(ranges::reserve_hint(@\exposid{base_}@));
//       return @\exposid{to-unsigned-like}@(s < @\exposid{count_}@ ? 0 : s - @\exposid{count_}@);
//     }
//
    // clang-format on
    std::ranges::drop_view value(base, count);
    if (!check_hint(value, hint < count ? 0 : hint - count))
      return false;
  }
  {
    using Base = ModuleHintView<int, Sized>;
    Base base{{}, storage};
    // clang-format off
// Adopted draft: as_rvalue_view reserve_hint (raw clause text).
//     constexpr auto reserve_hint() requires @\libconcept{approximately_sized_range}@<V>
//     { return ranges::reserve_hint(@\exposid{base_}@); }
//     constexpr auto reserve_hint() const requires @\libconcept{approximately_sized_range}@<const V>
//     { return ranges::reserve_hint(@\exposid{base_}@); }
    // clang-format on
    std::ranges::as_rvalue_view value(base);
    if (!check_hint(value, hint))
      return false;
  }
  {
    using Base = ModuleHintView<int, Sized>;
    Base base{{}, storage};
    // clang-format off
// Adopted draft: concat_view reserve_hint (raw clause text).
// constexpr auto reserve_hint() requires (@\libconcept{approximately_sized_range}@<Views> && ...);
// constexpr auto reserve_hint() const requires (@\libconcept{approximately_sized_range}@<const Views> && ...);
// Equivalent to:
// \begin{codeblock}
// return apply(
//   [](auto... sizes) {
//     using CT = @\exposid{make-unsigned-like-t}@<common_type_t<decltype(sizes)...>>;
//     return (CT(sizes) + ...);
//   },
//   @\exposid{tuple-transform}@(ranges::reserve_hint, @\exposid{views_}@));
    // clang-format on
    std::ranges::concat_view value(base, base);
    if (!check_hint(value, 2 * hint))
      return false;
  }
  {
    using Base = ModuleHintView<int, Sized>;
    Base base{{}, storage};
    // clang-format off
// Adopted draft: common_view reserve_hint (raw clause text).
//     constexpr auto reserve_hint() requires @\libconcept{approximately_sized_range}@<V> {
//       return ranges::reserve_hint(@\exposid{base_}@);
//     }
//     constexpr auto reserve_hint() const requires @\libconcept{approximately_sized_range}@<const V> {
//       return ranges::reserve_hint(@\exposid{base_}@);
//     }
    // clang-format on
    std::ranges::common_view value(base);
    if (!check_hint(value, hint))
      return false;
  }
  {
    using Base = ModuleHintView<int, Sized>;
    Base base{{}, storage};
    // clang-format off
// Adopted draft: reverse_view reserve_hint (raw clause text).
//     constexpr auto reserve_hint() requires @\libconcept{approximately_sized_range}@<V> {
//       return ranges::reserve_hint(@\exposid{base_}@);
//     }
//     constexpr auto reserve_hint() const requires @\libconcept{approximately_sized_range}@<const V> {
//       return ranges::reserve_hint(@\exposid{base_}@);
//     }
    // clang-format on
    std::ranges::reverse_view value(base);
    if (!check_hint(value, hint))
      return false;
  }
  {
    using Base = ModuleHintView<int, Sized>;
    Base base{{}, storage};
    // clang-format off
// Adopted draft: as_const_view reserve_hint (raw clause text).
//     constexpr auto reserve_hint() requires @\libconcept{approximately_sized_range}@<V>
//     { return ranges::reserve_hint(@\exposid{base_}@); }
//     constexpr auto reserve_hint() const requires @\libconcept{approximately_sized_range}@<const V>
//     { return ranges::reserve_hint(@\exposid{base_}@); }
    // clang-format on
    std::ranges::as_const_view value(base);
    if (!check_hint(value, hint))
      return false;
  }
  {
    using Base = ModuleHintView<std::tuple<int>, Sized>;
    Base base{{}, tuples};
    // clang-format off
// Adopted draft: elements_view reserve_hint (raw clause text).
//     constexpr auto reserve_hint() requires @\libconcept{approximately_sized_range}@<V>
//     { return ranges::reserve_hint(@\exposid{base_}@); }
//
//     constexpr auto reserve_hint() const requires @\libconcept{approximately_sized_range}@<const V>
//     { return ranges::reserve_hint(@\exposid{base_}@); }
    // clang-format on
    std::ranges::elements_view<Base, 0> value(base);
    if (!check_hint(value, hint))
      return false;
  }
  {
    using Base = ModuleHintView<int, Sized>;
    Base base{{}, storage};
    // clang-format off
// Adopted draft: enumerate_view reserve_hint (raw clause text).
//     constexpr auto reserve_hint() requires @\libconcept{approximately_sized_range}@<V>
//     { return ranges::reserve_hint(@\exposid{base_}@); }
//     constexpr auto reserve_hint() const requires @\libconcept{approximately_sized_range}@<const V>
//     { return ranges::reserve_hint(@\exposid{base_}@); }
    // clang-format on
    std::ranges::enumerate_view value(base);
    if (!check_hint(value, hint))
      return false;
  }
  {
    using Base = ModuleHintView<int, Sized>;
    Base base{{}, storage};
    // clang-format off
// Adopted draft: adjacent_view reserve_hint (raw clause text).
// constexpr auto reserve_hint() requires @\libconcept{approximately_sized_range}@<V>;
// constexpr auto reserve_hint() const requires @\libconcept{approximately_sized_range}@<const V>;
// using DT = range_difference_t<decltype((@\exposid{base_}@))>;
// using CT = common_type_t<DT, size_t>;
// auto sz = static_cast<CT>(ranges::reserve_hint(@\exposid{base_}@));
// sz -= std::min<CT>(sz, N - 1);
// return @\exposid{to-unsigned-like}@(sz);
    // clang-format on
    std::ranges::adjacent_view<Base, 2> value(base);
    if (!check_hint(value, hint < 1 ? 0 : hint - 1))
      return false;
  }
  {
    using Base = ModuleHintView<int, Sized>;
    Base base{{}, storage};
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
    std::ranges::adjacent_transform_view<Base, Add, 2> value(base, Add{});
    if (!check_hint(value, hint < 1 ? 0 : hint - 1))
      return false;
  }
  {
    using Base = ModuleHintView<int, Sized>;
    Base base{{}, storage};
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
    std::ranges::chunk_view value(base, count);
    if (!check_hint(value, hint / count + (hint % count != 0)))
      return false;
  }
  {
    using Base = ModuleHintView<int, Sized>;
    Base base{{}, storage};
    // clang-format off
// Adopted draft: stride_view reserve_hint (raw clause text).
// constexpr auto reserve_hint() requires @\libconcept{approximately_sized_range}@<V>;
// constexpr auto reserve_hint() const requires @\libconcept{approximately_sized_range}@<const V>;
// auto s = static_cast<range_difference_t<decltype((@\exposid{base_}@))>>(ranges::reserve_hint(@\exposid{base_}@));
// return @\exposid{to-unsigned-like}@(@\exposid{div-ceil}@(s, @\exposid{stride_}@));
    // clang-format on
    std::ranges::stride_view value(base, count);
    if (!check_hint(value, hint / count + (hint % count != 0)))
      return false;
  }
  {
    using Base = ModuleHintView<int, Sized>;
    Base base{{}, storage};
    // clang-format off
// Adopted draft: cache_latest_view reserve_hint (raw clause text).
// constexpr auto reserve_hint() requires @\libconcept{approximately_sized_range}@<V>;
// constexpr auto reserve_hint() const requires @\libconcept{approximately_sized_range}@<const V>;
// Equivalent to: \tcode{return ranges::reserve_hint(\exposid{base_});}
    // clang-format on
    std::ranges::cache_latest_view value(base);
    if (!check_hint(value, hint))
      return false;
  }
  {
    using Base = ModuleHintView<int, Sized>;
    Base base{{}, storage};
    // clang-format off
// Adopted draft: as_input_view reserve_hint (raw clause text).
// constexpr auto reserve_hint() requires @\libconcept{approximately_sized_range}@<V>;
// constexpr auto reserve_hint() const requires @\libconcept{approximately_sized_range}@<const V>;
// Equivalent to: \tcode{return ranges::reserve_hint(\exposid{base_});}
    // clang-format on
    std::ranges::as_input_view value(base);
    if (!check_hint(value, hint))
      return false;
  }
  return true;
}

struct InputIterator {
  using value_type      = int;
  using difference_type = std::ptrdiff_t;
  int* position;
  constexpr int& operator*() const { return *position; }
  constexpr InputIterator& operator++() {
    ++position;
    return *this;
  }
  constexpr void operator++(int) { ++position; }
  friend constexpr bool operator==(InputIterator, InputIterator) = default;
};
struct ModuleInputView : std::ranges::view_interface<ModuleInputView> {
  int* first;
  constexpr InputIterator begin() const { return {first}; }
  constexpr InputIterator end() const { return {first + 5}; }
};
// clang-format off
//     constexpr V base() && { return std::move(@\exposid{base_}@); }
//
//     constexpr const Pred& pred() const;
//
//     constexpr @\exposid{iterator}@<false> begin();
//     constexpr @\exposid{iterator}@<true> begin() const
//       requires (@\libconcept{input_range}@<const V> && !@\libconcept{forward_range}@<const V> &&
//                 @\libconcept{indirect_unary_predicate}@<const Pred, iterator_t<const V>>);
//
//     constexpr auto end() {
//       if constexpr (@\libconcept{common_range}@<V>)
//         return @\exposid{iterator}@<false>{*this, ranges::end(@\exposid{base_}@)};
//       else
//         return @\exposid{sentinel}@<false>{*this};
//     }
//     constexpr @\exposid{sentinel}@<true> end() const
//       requires (@\libconcept{input_range}@<const V> && !@\libconcept{forward_range}@<const V> &&
//                 @\libconcept{indirect_unary_predicate}@<const Pred, iterator_t<const V>>) {
//       return @\exposid{sentinel}@<true>{*this};
//     }
//   };
//
// clang-format on
constexpr bool test_filter() {
  int storage[] = {1, 2, 3, 4, 5};
  const std::ranges::filter_view value(ModuleInputView{{}, storage}, [](int n) { return n % 2 == 0; });
  static_assert(std::ranges::input_range<decltype(value)>);
  static_assert(!std::ranges::common_range<decltype(value)>);
  return std::ranges::distance(value) == 2;
}
// clang-format off
// constexpr auto reserve_hint() const noexcept;
// return @\exposid{to-unsigned-like}@(@\exposid{parent_}@->@\exposid{remainder_}@);
// clang-format on
constexpr bool test_input_chunk() {
  int storage[5]{};
  std::ranges::chunk_view value(ModuleInputView{{}, storage}, 3);
  auto outer = value.begin();
  auto inner = *outer;
  static_assert(noexcept(inner.reserve_hint()));
  if (inner.reserve_hint() != 3)
    return false;
  auto position = inner.begin();
  ++position;
  return inner.reserve_hint() == 2;
}
int main(int, char**) {
  static_assert(test_hints<false>());
  static_assert(test_hints<true>());
  static_assert(test_filter());
  static_assert(test_input_chunk());
  return test_hints<false>() && test_hints<true>() && test_filter() && test_input_chunk() ? 0 : 1;
}
