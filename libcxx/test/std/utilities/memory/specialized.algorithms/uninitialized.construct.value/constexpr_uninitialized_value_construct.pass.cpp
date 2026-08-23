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
// constexpr void uninitialized_value_construct(ForwardIt, ForwardIt);
// template <class ForwardIt, class Size>
// constexpr ForwardIt uninitialized_value_construct_n(ForwardIt, Size);
// namespace ranges { ... } // constexpr overloads, see [uninitialized.construct.value]
//
// Made constexpr by P3369R0/P3508R0. See the constexpr_uninitialized_default_
// construct.pass.cpp test in the sibling directory for why these tests must
// go through std::allocator rather than a raw byte buffer.

#include <memory>

#include <cassert>
#include <cstddef>
#include <ranges>

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
  return with_allocated<int>(3, [](int* p, std::size_t n) {
    std::uninitialized_value_construct(p, p + n);
    return p[0] == 0 && p[1] == 0 && p[2] == 0;
  });
}
static_assert(test_classic());

constexpr bool test_classic_n() {
  return with_allocated<int>(3, [](int* p, std::size_t n) {
    auto result = std::uninitialized_value_construct_n(p, n);
    return result == p + n && p[0] == 0;
  });
}
static_assert(test_classic_n());

constexpr bool test_ranges_iter() {
  return with_allocated<int>(3, [](int* p, std::size_t n) {
    auto result = std::ranges::uninitialized_value_construct(p, p + n);
    return result == p + n && p[0] == 0;
  });
}
static_assert(test_ranges_iter());

constexpr bool test_ranges_range() {
  return with_allocated<int>(3, [](int* p, std::size_t n) {
    std::ranges::subrange r(p, p + n);
    auto result = std::ranges::uninitialized_value_construct(r);
    return result == p + n && p[0] == 0;
  });
}
static_assert(test_ranges_range());

constexpr bool test_ranges_n() {
  return with_allocated<int>(3, [](int* p, std::size_t n) {
    auto result = std::ranges::uninitialized_value_construct_n(p, n);
    return result == p + n && p[0] == 0;
  });
}
static_assert(test_ranges_n());

int main(int, char**) { return 0; }
