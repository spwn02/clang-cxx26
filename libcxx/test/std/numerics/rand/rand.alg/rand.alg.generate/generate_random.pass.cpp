//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <random>

// namespace ranges {
//   template<class R, class G>
//     requires output_range<R, invoke_result_t<G&>> &&
//              uniform_random_bit_generator<remove_cvref_t<G>>
//     constexpr borrowed_iterator_t<R> generate_random(R&& r, G&& g);
//
//   template<class G, output_iterator<invoke_result_t<G&>> O, sentinel_for<O> S>
//     requires uniform_random_bit_generator<remove_cvref_t<G>>
//     constexpr O generate_random(O first, S last, G&& g);
//
//   template<class R, class G, class D>
//     requires output_range<R, invoke_result_t<D&, G&>> &&
//              invocable<D&, G&> &&
//              uniform_random_bit_generator<remove_cvref_t<G>>
//     constexpr borrowed_iterator_t<R> generate_random(R&& r, G&& g, D&& d);
//
//   template<class G, class D, output_iterator<invoke_result_t<D&, G&>> O, sentinel_for<O> S>
//     requires invocable<D&, G&> &&
//              uniform_random_bit_generator<remove_cvref_t<G>>
//     constexpr O generate_random(O first, S last, G&& g, D&& d);
// }

#include <array>
#include <cassert>
#include <concepts>
#include <random>
#include <ranges>
#include <utility>

#include "test_iterators.h"

// A minimal `uniform_random_bit_generator` usable in `constexpr` contexts.
struct SimpleGen {
  using result_type = unsigned;
  static constexpr result_type min() { return 0; }
  static constexpr result_type max() { return 100; }
  constexpr result_type operator()() { return ++counter_; }
  unsigned counter_ = 0;
};
static_assert(std::uniform_random_bit_generator<SimpleGen>);

// A generator that customizes `generate_random` via a member function.
struct CustomGen {
  using result_type = unsigned;
  static constexpr result_type min() { return 0; }
  static constexpr result_type max() { return 100; }
  constexpr result_type operator()() { return ++counter_; }

  template <class R>
  constexpr void generate_random(R&& r) {
    used_member_ = true;
    for (auto& e : r)
      e = 42;
  }

  unsigned counter_     = 0;
  bool used_member_     = false;
};
static_assert(std::uniform_random_bit_generator<CustomGen>);

// A distribution that customizes `generate_random` via a member function.
struct CustomDist {
  template <class R, class G>
  constexpr void generate_random(R&& r, G&) {
    used_member_ = true;
    for (auto& e : r)
      e = 7;
  }

  template <class G>
  constexpr int operator()(G& g) {
    return static_cast<int>(g());
  }

  bool used_member_ = false;
};

struct NotInvocable {};

// Test constraints of the (range, generator) overload.
// ======================================================

template <class R, class G>
concept HasGenerateRandomRange = requires(R&& r, G&& g) {
  std::ranges::generate_random(std::forward<R>(r), std::forward<G>(g));
};

static_assert(HasGenerateRandomRange<std::array<unsigned, 3>&, SimpleGen&>);
static_assert(!HasGenerateRandomRange<std::array<unsigned, 3>&, NotInvocable&>);
// !uniform_random_bit_generator<G>: not invocable at all.
static_assert(!HasGenerateRandomRange<std::array<unsigned, 3>&, int&>);

// Test constraints of the (iterator, sentinel, generator) overload.
// ===================================================================

template <class O, class S, class G>
concept HasGenerateRandomIter = requires(O first, S last, G&& g) {
  std::ranges::generate_random(first, last, std::forward<G>(g));
};

static_assert(HasGenerateRandomIter<unsigned*, unsigned*, SimpleGen&>);
static_assert(!HasGenerateRandomIter<unsigned*, unsigned*, NotInvocable&>);

// Test constraints of the (range, generator, distribution) overload.
// ======================================================================

template <class R, class G, class D>
concept HasGenerateRandomRangeDist = requires(R&& r, G&& g, D&& d) {
  std::ranges::generate_random(std::forward<R>(r), std::forward<G>(g), std::forward<D>(d));
};

static_assert(HasGenerateRandomRangeDist<std::array<int, 3>&, SimpleGen&, std::uniform_int_distribution<int>&>);
static_assert(!HasGenerateRandomRangeDist<std::array<int, 3>&, NotInvocable&, std::uniform_int_distribution<int>&>);

constexpr bool test() {
  // (range, generator) overload: no customization -- falls back to `ranges::generate(r, ref(g))`.
  {
    std::array<unsigned, 5> out{};
    SimpleGen gen;
    std::same_as<unsigned*> decltype(auto) result = std::ranges::generate_random(out, gen);
    assert(result == out.data() + out.size());
    for (std::size_t i = 0; i < out.size(); ++i)
      assert(out[i] == i + 1);
    assert(gen.counter_ == out.size());
  }

  // (iterator, sentinel, generator) overload.
  {
    std::array<unsigned, 5> out{};
    SimpleGen gen;
    std::same_as<unsigned*> decltype(auto) result = std::ranges::generate_random(out.begin(), out.end(), gen);
    assert(result == out.data() + out.size());
    for (std::size_t i = 0; i < out.size(); ++i)
      assert(out[i] == i + 1);
  }

  // (range, generator) overload: generator customizes `generate_random`.
  {
    std::array<unsigned, 5> out{};
    CustomGen gen;
    auto result = std::ranges::generate_random(out, gen);
    assert(result == out.data() + out.size());
    assert(gen.used_member_);
    assert(gen.counter_ == 0); // operator() was never called directly
    for (unsigned e : out)
      assert(e == 42);
  }

  // (range, generator) overload, non-common range: both the fallback path and the
  // member-customization path must return `borrowed_iterator_t<R>` (i.e. `iterator_t<R>`),
  // not `sentinel_t<R>` -- those differ for a non-common range.
  {
    std::array<unsigned, 5> out{};
    auto make_range = [&out] {
      return std::ranges::subrange(out.data(), sentinel_wrapper<unsigned*>(out.data() + out.size()));
    };

    { // fallback path
      SimpleGen gen;
      std::same_as<unsigned*> decltype(auto) result = std::ranges::generate_random(make_range(), gen);
      assert(result == out.data() + out.size());
    }

    { // member-customization path
      CustomGen gen;
      std::same_as<unsigned*> decltype(auto) result = std::ranges::generate_random(make_range(), gen);
      assert(result == out.data() + out.size());
      assert(gen.used_member_);
    }
  }

  // (range, generator, distribution) overload: distribution customizes `generate_random`.
  {
    std::array<int, 5> out{};
    SimpleGen gen;
    CustomDist dist;
    auto result = std::ranges::generate_random(out, gen, dist);
    assert(result == out.data() + out.size());
    assert(dist.used_member_);
    for (int e : out)
      assert(e == 7);
  }

  return true;
}

// `uniform_int_distribution`'s constructor isn't `constexpr`, so this is exercised only at runtime.
void test_uniform_int_distribution() {
  // (range, generator, distribution) overload: no customization -- falls back to
  // `ranges::generate(r, [&d, &g] { return invoke(d, g); })`.
  {
    std::array<int, 5> out{};
    SimpleGen gen;
    std::uniform_int_distribution<int> dist(0, 100);
    std::same_as<int*> decltype(auto) result = std::ranges::generate_random(out, gen, dist);
    assert(result == out.data() + out.size());
    for (int e : out)
      assert(e >= 0 && e <= 100);
  }

  // (iterator, sentinel, generator, distribution) overload.
  {
    std::array<int, 5> out{};
    SimpleGen gen;
    std::uniform_int_distribution<int> dist(0, 100);
    auto result = std::ranges::generate_random(out.begin(), out.end(), gen, dist);
    assert(result == out.data() + out.size());
    for (int e : out)
      assert(e >= 0 && e <= 100);
  }
}

int main(int, char**) {
  test();
  static_assert(test());
  test_uniform_int_distribution();

  return 0;
}
