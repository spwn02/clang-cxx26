//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <memory>

// template <class ForwardIt>
// constexpr void uninitialized_default_construct(ForwardIt, ForwardIt);
// template <class ForwardIt, class Size>
// constexpr ForwardIt uninitialized_default_construct_n(ForwardIt, Size);
// namespace ranges {
//   template<nothrow-forward-iterator I, nothrow-sentinel-for<I> S>
//     requires default_initializable<iter_value_t<I>>
//   constexpr I uninitialized_default_construct(I, S);
//   template<nothrow-forward-range R>
//     requires default_initializable<range_value_t<R>>
//   constexpr borrowed_iterator_t<R> uninitialized_default_construct(R&&);
//   template<nothrow-forward-iterator I>
//     requires default_initializable<iter_value_t<I>>
//   constexpr I uninitialized_default_construct_n(I, iter_difference_t<I>);
// }
//
// [uninitialized.construct.default] is made constexpr by P3369R0/P3508R0.
// This requires allocator-obtained storage: reusing a declared object's
// storage for a different type (e.g. a stack char[] buffer) is not
// constexpr-usable even with P2747R2 support, so these tests must go through
// std::allocator, not a raw byte buffer like the runtime tests in this
// directory do.

#include <memory>

#include <cassert>
#include <cstddef>
#include <ranges>

struct Counted {
  int value = 5;
  constexpr Counted() = default;
};

template <class T, class F>
constexpr bool with_allocated(std::size_t n, F f) {
  std::allocator<T> alloc;
  T* p = alloc.allocate(n);
  bool result = f(p, n);
  std::destroy(p, p + n);
  alloc.deallocate(p, n);
  return result;
}

constexpr bool test_classic() {
  return with_allocated<Counted>(3, [](Counted* p, std::size_t n) {
    std::uninitialized_default_construct(p, p + n);
    return p[0].value == 5 && p[1].value == 5 && p[2].value == 5;
  });
}
static_assert(test_classic());

constexpr bool test_classic_n() {
  return with_allocated<Counted>(3, [](Counted* p, std::size_t n) {
    auto result = std::uninitialized_default_construct_n(p, n);
    return result == p + n && p[0].value == 5 && p[2].value == 5;
  });
}
static_assert(test_classic_n());

constexpr bool test_ranges_iter() {
  return with_allocated<Counted>(3, [](Counted* p, std::size_t n) {
    auto result = std::ranges::uninitialized_default_construct(p, p + n);
    return result == p + n && p[0].value == 5 && p[2].value == 5;
  });
}
static_assert(test_ranges_iter());

constexpr bool test_ranges_range() {
  return with_allocated<Counted>(3, [](Counted* p, std::size_t n) {
    std::ranges::subrange r(p, p + n);
    auto result = std::ranges::uninitialized_default_construct(r);
    return result == p + n && p[0].value == 5;
  });
}
static_assert(test_ranges_range());

constexpr bool test_ranges_n() {
  return with_allocated<Counted>(3, [](Counted* p, std::size_t n) {
    auto result = std::ranges::uninitialized_default_construct_n(p, n);
    return result == p + n && p[0].value == 5;
  });
}
static_assert(test_ranges_n());

int main(int, char**) { return 0; }
