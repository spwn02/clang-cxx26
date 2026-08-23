//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <memory>

// template <class InputIt, class ForwardIt>
// constexpr ForwardIt uninitialized_move(InputIt, InputIt, ForwardIt);
// template <class InputIt, class Size, class ForwardIt>
// constexpr pair<InputIt, ForwardIt> uninitialized_move_n(InputIt, Size, ForwardIt);
// namespace ranges { ... } // constexpr overloads, see [uninitialized.move]
//
// Made constexpr by P3369R0/P3508R0. See the constexpr_uninitialized_default_
// construct.pass.cpp test in a sibling directory for why the *output* storage
// must go through std::allocator rather than a raw byte buffer.

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
  int src[3] = {1, 2, 3};
  return with_allocated<int>(3, [&](int* p, std::size_t n) {
    auto result = std::uninitialized_move(src, src + n, p);
    return result == p + n && p[0] == 1 && p[1] == 2 && p[2] == 3;
  });
}
static_assert(test_classic());

constexpr bool test_classic_n() {
  int src[3] = {1, 2, 3};
  return with_allocated<int>(3, [&](int* p, std::size_t n) {
    auto result = std::uninitialized_move_n(src, n, p);
    return result.second == p + n && p[0] == 1;
  });
}
static_assert(test_classic_n());

constexpr bool test_ranges_iter() {
  int src[3] = {1, 2, 3};
  return with_allocated<int>(3, [&](int* p, std::size_t n) {
    auto result = std::ranges::uninitialized_move(src, src + n, p, p + n);
    return result.out == p + n && p[0] == 1;
  });
}
static_assert(test_ranges_iter());

constexpr bool test_ranges_range() {
  int src[3] = {1, 2, 3};
  return with_allocated<int>(3, [&](int* p, std::size_t n) {
    std::ranges::subrange in(src, src + n);
    std::ranges::subrange out(p, p + n);
    auto result = std::ranges::uninitialized_move(in, out);
    return result.out == p + n && p[0] == 1;
  });
}
static_assert(test_ranges_range());

constexpr bool test_ranges_n() {
  int src[3] = {1, 2, 3};
  return with_allocated<int>(3, [&](int* p, std::size_t n) {
    auto result = std::ranges::uninitialized_move_n(src, n, p, p + n);
    return result.out == p + n && p[0] == 1;
  });
}
static_assert(test_ranges_n());

int main(int, char**) { return 0; }
