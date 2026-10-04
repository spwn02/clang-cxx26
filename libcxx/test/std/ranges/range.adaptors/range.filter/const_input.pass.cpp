//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

#include <ranges>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>

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

//     using @\exposidnc{Parent}@ = @\exposidnc{maybe-const}@<Const, filter_view>;                     // \expos
//     using @\exposidnc{Base}@ = @\exposidnc{maybe-const}@<Const, V>;                                 // \expos
//     iterator_t<@\exposidnc{Base}@> @\exposid{current_}@ = iterator_t<@\exposidnc{Base}@>();                     // \expos
//     @\exposidnc{Parent}@* @\exposid{parent_}@ = nullptr;                                          // \expos
//     constexpr @\exposidnc{iterator}@(@\exposidnc{Parent}@& parent, iterator_t<@\exposidnc{Base}@> current);       // \expos
//
//   public:
//     using iterator_concept  = @\seebelownc@;
//     using iterator_category = @\seebelownc@;                                // not always present
//     using value_type        = range_value_t<@\exposidnc{Base}@>;
//     using difference_type   = range_difference_t<@\exposidnc{Base}@>;
//
//     @\exposid{iterator}@() requires @\libconcept{default_initializable}@<iterator_t<@\exposidnc{Base}@>> = default;
//     constexpr @\exposid{iterator}@(iterator<!Const> i)
//       requires Const && @\libconcept{convertible_to}@<iterator_t<V>, iterator_t<@\exposidnc{Base}@>>;
//
//     constexpr const iterator_t<@\exposidnc{Base}@>& base() const & noexcept;
//     constexpr iterator_t<@\exposidnc{Base}@> base() &&;
//     constexpr range_reference_t<@\exposidnc{Base}@> operator*() const;
//     constexpr iterator_t<@\exposidnc{Base}@> operator->() const
//       requires @\exposconcept{has-arrow}@<iterator_t<@\exposidnc{Base}@>> && @\libconcept{copyable}@<iterator_t<@\exposidnc{Base}@>>;

// \item If \tcode{Const} is \tcode{true},
// then \tcode{iterator_concept} denotes \tcode{input_iterator_tag}.
//
// \item Otherwise, if \tcode{V} models \libconcept{bidirectional_range}, then
// \tcode{iterator_concept} denotes \tcode{bidirectional_iterator_tag}.
//
// \item Otherwise, if \tcode{V} models \libconcept{forward_range}, then
// \tcode{iterator_concept} denotes \tcode{forward_iterator_tag}.
//
// \item Otherwise, \tcode{iterator_concept} denotes \tcode{input_iterator_tag}.
// \end{itemize}
//
// \pnum
// The member \grammarterm{typedef-name} \tcode{iterator_category} is declared
// if and only if \exposid{Base} models \libconcept{forward_range}.
// In that case,
// \tcode{\exposid{iterator}::iterator_category} is defined as follows:
// \begin{itemize}
// \item Let \tcode{C} denote the type
// \tcode{iterator_traits<iterator_t<\exposid{Base}>>::iterator_category}.
//
// \item If \tcode{C} models
// \tcode{\libconcept{derived_from}<bidirectional_iterator_tag>},
// then \tcode{iterator_category} denotes \tcode{bi\-directional_iterator_tag}.
//
// \item Otherwise, if  \tcode{C} models
// \tcode{\libconcept{derived_from}<forward_iterator_tag>},
// then \tcode{iterator_category} denotes \tcode{forward_iterator_tag}.
//
// \item Otherwise, \tcode{iterator_category} denotes \tcode{C}.

//     using @\exposidnc{Base}@ = @\exposidnc{maybe-const}@<Const, V>;                         // \expos
//     sentinel_t<@\exposidnc{Base}@> @\exposid{end_}@ = sentinel_t<@\exposidnc{Base}@>();                 // \expos
//     constexpr explicit @\exposidnc{sentinel}@(@\exposidnc{Parent}@& parent);                // \expos
//
//   public:
//     @\exposid{sentinel}@() = default;
//     constexpr @\exposid{sentinel}@(@\exposid{sentinel}@<!Const> other)
//       requires Const && @\libconcept{convertible_to}@<sentinel_t<V>, sentinel_t<@\exposidnc{Base}@>>;
//
//     constexpr sentinel_t<@\exposidnc{Base}@> base() const;
//
//     template<bool OtherConst>
//       requires @\libconcept{sentinel_for}@<sentinel_t<@\exposidnc{Base}@>, iterator_t<@\exposid{maybe-const}@<OtherConst, V>>>
//       friend constexpr bool operator==(const @\exposid{iterator}@<OtherConst>& x, const @\exposid{sentinel}@& y);
//   };
// }
// \end{codeblock}
//
// \indexlibraryctor{filter_view::\exposid{sentinel}}%
// \begin{itemdecl}

// \pnum
// \expects
// \tcode{\exposid{pred_}.has_value()} is \tcode{true}.
//
// \pnum
// \returns
// \tcode{\{*this, ranges::find_if(\exposid{base_}, ref(*\exposid{pred_}))\}}.
// \begin{note}
// This function does not cache the result within the \tcode{filter_view}.
// \end{note}
// \end{itemdescr}

// clang-format on
template <bool Const>
struct InputIterator {
  using value_type          = int;
  using difference_type     = std::ptrdiff_t;
  using iterator_concept    = std::input_iterator_tag;
  using Pointer             = std::conditional_t<Const, const int*, int*>;
  Pointer position          = nullptr;
  constexpr InputIterator() = default;
  constexpr InputIterator(Pointer p) : position(p) {}
  constexpr InputIterator(InputIterator<!Const> other)
    requires Const
      : position(other.position) {}
  constexpr decltype(auto) operator*() const { return *position; }
  constexpr Pointer operator->() const { return position; }
  constexpr InputIterator& operator++() {
    ++position;
    return *this;
  }
  constexpr void operator++(int) { ++position; }
  friend constexpr bool operator==(InputIterator, InputIterator) = default;
};

template <bool Const>
struct Sentinel {
  using Pointer        = std::conditional_t<Const, const int*, int*>;
  Pointer position     = nullptr;
  constexpr Sentinel() = default;
  constexpr Sentinel(Pointer p) : position(p) {}
  constexpr Sentinel(Sentinel<!Const> other)
    requires Const
      : position(other.position) {}
  template <bool OtherConst>
  friend constexpr bool operator==(InputIterator<OtherConst> iter, Sentinel sent) {
    return iter.position == sent.position;
  }
};

template <bool Common, bool MutableForward = false>
struct InputView : std::ranges::view_interface<InputView<Common, MutableForward>> {
  int* first            = nullptr;
  int* last             = nullptr;
  constexpr InputView() = default;
  constexpr InputView(int* f, int* l) : first(f), last(l) {}
  constexpr auto begin() {
    if constexpr (MutableForward)
      return first;
    else
      return InputIterator<false>(first);
  }
  constexpr auto end() {
    if constexpr (MutableForward)
      return last;
    else if constexpr (Common)
      return InputIterator<false>(last);
    else
      return Sentinel<false>(last);
  }
  constexpr auto begin() const { return InputIterator<true>(first); }
  constexpr auto end() const {
    if constexpr (Common)
      return InputIterator<true>(last);
    else
      return Sentinel<true>(last);
  }
};

struct Even {
  constexpr bool operator()(int value) const { return value % 2 == 0; }
};
struct MutablePredicate {
  bool operator()(int value) { return value % 2 == 0; }
};
struct CountingPredicate {
  int* calls;
  constexpr bool operator()(int value) const {
    ++*calls;
    return value % 2 == 0;
  }
};
template <class Iter>
concept HasIteratorCategory = requires { typename Iter::iterator_category; };

template <bool Common>
constexpr bool test_input() {
  int storage[] = {1, 2, 3, 4, 5};
  using Base    = InputView<Common>;
  using View    = std::ranges::filter_view<Base, Even>;
  static_assert(std::ranges::input_range<const View>);
  static_assert(!std::ranges::forward_range<const View>);
  static_assert(!std::ranges::common_range<const View>);
  using Iter      = std::ranges::iterator_t<View>;
  using ConstIter = std::ranges::iterator_t<const View>;
  using ConstSent = std::ranges::sentinel_t<const View>;
  static_assert(std::same_as<typename ConstIter::iterator_concept, std::input_iterator_tag>);
  static_assert(!HasIteratorCategory<ConstIter>);
  static_assert(!HasIteratorCategory<Iter>);
  static_assert(std::convertible_to<Iter, ConstIter>);
  static_assert(!std::convertible_to<ConstIter, Iter>);
  static_assert(std::same_as<decltype(std::declval<ConstIter&>()++), void>);
  static_assert(std::same_as<decltype(*std::declval<ConstIter&>()), const int&>);
  static_assert(std::sentinel_for<ConstSent, Iter>);
  static_assert(std::same_as<decltype(std::ranges::iter_move(std::declval<ConstIter&>())), const int&&>);
  if constexpr (!Common) {
    using Sent = std::ranges::sentinel_t<View>;
    static_assert(std::convertible_to<Sent, ConstSent>);
    static_assert(!std::convertible_to<ConstSent, Sent>);
    static_assert(std::sentinel_for<Sent, ConstIter>);
  }
  View value(Base(storage, storage + 5), Even{});
  const auto& const_value = value;
  auto current            = const_value.begin();
  assert(*current == 2);
  assert(current.operator->() == storage + 1);
  assert(current.base().position == storage + 1);
  ConstIter converted(value.begin());
  assert(converted == current);
  assert(converted != const_value.end());
  assert(const_value.end() != converted);
  ++current;
  assert(*current == 4);
  current++;
  assert(current == const_value.end());
  assert(std::ranges::distance(const_value) == 2);
  if constexpr (!Common) {
    ConstSent converted_end(value.end());
    assert(current == converted_end);
    assert(current == value.end());
    assert(converted_end.base().position == storage + 5);
  }
  auto base = std::move(converted).base();
  assert(*base == 2);
  View empty(Base(storage, storage), Even{});
  assert(std::as_const(empty).begin() == std::as_const(empty).end());
  View none(Base(storage, storage + 1), Even{});
  assert(std::as_const(none).begin() == std::as_const(none).end());
  return true;
}

constexpr bool test_cache_and_forward_base() {
  int storage[] = {1, 2, 3, 4};
  int calls     = 0;
  using Base    = InputView<true, true>;
  std::ranges::filter_view value(Base(storage, storage + 4), CountingPredicate{&calls});
  using Iter      = std::ranges::iterator_t<decltype(value)>;
  using ConstIter = std::ranges::iterator_t<const decltype(value)>;
  static_assert(std::same_as<typename Iter::iterator_concept, std::bidirectional_iterator_tag>);
  static_assert(std::same_as<typename Iter::iterator_category, std::bidirectional_iterator_tag>);
  static_assert(!HasIteratorCategory<ConstIter>);
  static_assert(std::same_as<typename ConstIter::iterator_concept, std::input_iterator_tag>);
  static_assert(std::convertible_to<Iter, ConstIter>);
  assert(*value.begin() == 2);
  assert(calls == 2);
  assert(*value.begin() == 2);
  assert(calls == 2);
  assert(*std::as_const(value).begin() == 2);
  assert(calls == 4);
  assert(*std::as_const(value).begin() == 2);
  assert(calls == 6);
  ConstIter converted(value.begin());
  assert(*converted == 2);
  return true;
}

using MutablePredView = std::ranges::filter_view<InputView<false>, MutablePredicate>;
static_assert(!std::ranges::range<const MutablePredView>);
using ForwardBase = std::ranges::ref_view<int[3]>;
using ForwardView = std::ranges::filter_view<ForwardBase, Even>;
static_assert(!std::ranges::range<const ForwardView>);

int main(int, char**) {
  test_input<false>();
  test_input<true>();
  test_cache_and_forward_base();
  static_assert(test_input<false>());
  static_assert(test_input<true>());
  static_assert(test_cache_and_forward_base());
  return 0;
}
