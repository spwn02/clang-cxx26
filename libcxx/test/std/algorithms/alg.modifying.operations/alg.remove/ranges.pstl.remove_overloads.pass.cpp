//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: libcpp-has-no-incomplete-pstl

#include <algorithm>
#include <array>
#include <cassert>
#include <execution>
#include <span>
#include <type_traits>
#include <utility>

struct record {
  int value;
};
struct is_one {
  bool operator()(int value) const { return value == 1; }
};

template <class Element>
struct unsized_sentinel {
  Element* last;
  friend bool operator==(Element* iter, unsized_sentinel sent) { return iter == sent.last; }
};

template <class Element>
struct sized_unsized_range {
  Element* first;
  std::size_t count;
  Element* begin() const { return first; }
  unsized_sentinel<Element> end() const { return {first + count}; }
  std::size_t size() const { return count; }
};

// [range.refinements]: "concept sized-random-access-range = random_access_range<T> && sized_range<T>;"
static_assert(std::ranges::random_access_range<sized_unsized_range<record>>);
static_assert(std::ranges::sized_range<sized_unsized_range<record>>);
static_assert(!std::sized_sentinel_for<unsized_sentinel<record>, record*>);

template <class Range>
concept can_remove_if =
    requires(Range&& range) { std::ranges::remove_if(std::execution::seq, std::forward<Range>(range), is_one{}); };
// [alg.remove]: "requires permutable<iterator_t<R>>"
static_assert(can_remove_if<std::span<int>>);
static_assert(!can_remove_if<std::span<const int>>);

// [alg.remove]: "borrowed_subrange_t<R> ranges::remove_if(Ep&& exec, R&& r, Pred pred, Proj proj = {});"
static_assert(std::is_same_v<decltype(std::ranges::remove_if(std::execution::seq, std::array<int, 3>{}, is_one{})),
                             std::ranges::dangling>);
// [alg.remove]: "remove_copy_result<borrowed_iterator_t<R>, borrowed_iterator_t<OutR>>"
static_assert(std::is_same_v<
              decltype(std::ranges::remove_copy(std::execution::seq, std::array<int, 3>{}, std::array<int, 3>{}, 1)),
              std::ranges::remove_copy_result<std::ranges::dangling, std::ranges::dangling>>);
// [alg.remove]: "remove_copy_if_result<borrowed_iterator_t<R>, borrowed_iterator_t<OutR>>"
static_assert(std::is_same_v<decltype(std::ranges::remove_copy_if(
                                 std::execution::seq, std::array<int, 3>{}, std::array<int, 3>{}, is_one{})),
                             std::ranges::remove_copy_if_result<std::ranges::dangling, std::ranges::dangling>>);

template <class Policy>
void test(Policy&& policy) {
  // [alg.remove]: "Eliminates all the elements referred to by iterator i in the range [first, last)
  // for which E(i) holds." "{j, last} for the overloads in namespace ranges."
  std::array<record, 5> range{{{1}, {2}, {1}, {3}, {1}}};
  auto removed = std::ranges::remove_if(policy, range, is_one{}, &record::value);
  assert(removed.begin() == range.begin() + 2 && removed.end() == range.end());
  assert(range[0].value == 2 && range[1].value == 3);
  std::array<int, 3> iter_range{1, 2, 1};
  auto iter_removed = std::ranges::remove_if(policy, iter_range.begin(), iter_range.end(), is_one{});
  assert(iter_removed.begin() == iter_range.begin() + 1 && iter_removed.end() == iter_range.end());

  // [alg.remove]: "N be min(M, result_last - result)."
  // "Copies the first N elements referred to by the iterator i in the range [first, last)
  // for which E(i) is false into the range [result, result + N)."
  // "Otherwise, {j, result_last}, ... where j is the iterator in [first, last)
  // for which E(j) is false and there are exactly N iterators i in [first, j) for which E(i) is false."
  std::array<record, 6> input{{{1}, {2}, {1}, {3}, {1}, {4}}};
  std::array<record, 3> output{{{-9}, {-9}, {-9}}};
  std::span<record> short_output(output.data(), 2);
  auto copied = std::ranges::remove_copy(policy, input, short_output, {1}, &record::value);
  assert(copied.in == input.begin() + 5 && copied.out == short_output.end());
  assert(output[0].value == 2 && output[1].value == 3 && output[2].value == -9);
  output         = {{{-9}, {-9}, {-9}}};
  auto copied_if = std::ranges::remove_copy_if(policy, input, short_output, is_one{}, &record::value);
  assert(copied_if.in == input.begin() + 5 && copied_if.out == short_output.end());
  assert(output[0].value == 2 && output[1].value == 3 && output[2].value == -9);

  // [alg.remove]: "{last, result + N}, ... if N is equal to M."
  auto full =
      std::ranges::remove_copy(policy, input.begin(), input.end(), output.begin(), output.end(), {1}, &record::value);
  assert(full.in == input.end() && full.out == output.end() && output[2].value == 4);
  auto full_if = std::ranges::remove_copy_if(
      policy, input.begin(), input.end(), output.begin(), output.end(), is_one{}, &record::value);
  assert(full_if.in == input.end() && full_if.out == output.end());
  // [alg.remove]: "j ... for which E(j) is false" also when N is zero.
  std::span<record> empty_output(output.data(), 0);
  auto empty = std::ranges::remove_copy_if(policy, input, empty_output, is_one{}, &record::value);
  assert(empty.in == input.begin() + 1 && empty.out == empty_output.begin());
  std::array<int, 3> all_removed{1, 1, 1};
  std::span<int> no_output;
  auto all = std::ranges::remove_copy(policy, all_removed, no_output, 1);
  assert(all.in == all_removed.end() && all.out == no_output.begin());

  // [algorithms.requirements]: "a corresponding sentinel argument is initialized with ranges::end(r),
  // or ranges::begin(r) + N where N is equal to ranges::distance(r)."
  // [alg.remove]: "{j, last} for the overloads in namespace ranges."
  std::array<record, 5> unsized_storage{{{1}, {2}, {1}, {3}, {1}}};
  sized_unsized_range<record> unsized_input{unsized_storage.data(), unsized_storage.size()};
  auto unsized_removed = std::ranges::remove_if(policy, unsized_input, is_one{}, &record::value);
  assert(unsized_removed.begin() == unsized_storage.data() + 2);
  assert(unsized_removed.end() == unsized_storage.data() + 5);
  assert(unsized_storage[0].value == 2 && unsized_storage[1].value == 3);

  // [alg.remove]: "Otherwise, {j, result_last}, ... where j is the iterator in [first, last)
  // for which E(j) is false and there are exactly N iterators i in [first, j) for which E(i) is false."
  sized_unsized_range<record> unsized_copy_input{input.data(), input.size()};
  sized_unsized_range<record> unsized_output{output.data(), 2};
  output            = {{{-9}, {-9}, {-9}}};
  auto unsized_copy = std::ranges::remove_copy(policy, unsized_copy_input, unsized_output, {1}, &record::value);
  assert(unsized_copy.in == input.data() + 5 && unsized_copy.out == output.data() + 2);
  assert(output[0].value == 2 && output[1].value == 3 && output[2].value == -9);
  output = {{{-9}, {-9}, {-9}}};
  auto unsized_copy_if =
      std::ranges::remove_copy_if(policy, unsized_copy_input, unsized_output, is_one{}, &record::value);
  assert(unsized_copy_if.in == input.data() + 5 && unsized_copy_if.out == output.data() + 2);
  assert(output[0].value == 2 && output[1].value == 3 && output[2].value == -9);
}

int main(int, char**) {
  test(std::execution::seq);
  test(std::execution::par);
  test(std::execution::par_unseq);
  test(std::execution::unseq);
  return 0;
}
