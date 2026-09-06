//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// REQUIRES: has-unix-headers
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20
// UNSUPPORTED: libcpp-hardening-mode=none
// XFAIL: libcpp-hardening-mode=debug && availability-verbose_abort-missing

#include <cassert>
#include <cstddef>
#include <iterator>

#include "check_assertion.h"

#ifndef TEST_HAS_NO_EXCEPTIONS
struct ConvertibleIterator {
  using value_type      = int;
  using difference_type = std::ptrdiff_t;
  using pointer         = int*;
  using reference       = int&;
  using iterator_category = std::random_access_iterator_tag;
  using iterator_concept  = std::random_access_iterator_tag;

  int* ptr = nullptr;

  constexpr operator int*() const { return ptr; }
  constexpr int& operator*() const { return *ptr; }
  constexpr ConvertibleIterator& operator++() {
    ++ptr;
    return *this;
  }
  constexpr ConvertibleIterator operator++(int) {
    ConvertibleIterator result = *this;
    ++*this;
    return result;
  }
  constexpr ConvertibleIterator& operator--() {
    --ptr;
    return *this;
  }
  constexpr ConvertibleIterator operator--(int) {
    ConvertibleIterator result = *this;
    --*this;
    return result;
  }
  constexpr ConvertibleIterator& operator+=(difference_type n) {
    ptr += n;
    return *this;
  }
  constexpr ConvertibleIterator& operator-=(difference_type n) {
    ptr -= n;
    return *this;
  }

  friend constexpr ConvertibleIterator operator+(ConvertibleIterator i, difference_type n) { return i += n; }
  friend constexpr ConvertibleIterator operator+(difference_type n, ConvertibleIterator i) { return i += n; }
  friend constexpr ConvertibleIterator operator-(ConvertibleIterator i, difference_type n) { return i -= n; }
  friend constexpr difference_type operator-(ConvertibleIterator x, ConvertibleIterator y) { return x.ptr - y.ptr; }
  friend constexpr bool operator==(ConvertibleIterator x, ConvertibleIterator y) { return x.ptr == y.ptr; }
  friend constexpr auto operator<=>(ConvertibleIterator x, ConvertibleIterator y) { return x.ptr <=> y.ptr; }
};

struct ThrowingSentinel {
  static bool throw_on_copy;

  int* ptr = nullptr;

  constexpr ThrowingSentinel() = default;
  constexpr explicit ThrowingSentinel(int* p) : ptr(p) {}
  ThrowingSentinel(const ThrowingSentinel& other) {
    if (throw_on_copy)
      throw 42;
    ptr = other.ptr;
  }
  ThrowingSentinel& operator=(const ThrowingSentinel&) = default;

  template <class _Iter>
    requires requires(const _Iter& i) { static_cast<int*>(i); }
  friend constexpr bool operator==(const ThrowingSentinel& s, const _Iter& i) {
    return s.ptr == static_cast<int*>(i);
  }

  template <class _Iter>
    requires requires(const _Iter& i) { static_cast<int*>(i); }
  friend constexpr bool operator==(const _Iter& i, const ThrowingSentinel& s) {
    return s == i;
  }

  friend constexpr std::ptrdiff_t operator-(const ThrowingSentinel& s, int* i) { return s.ptr - i; }
  friend constexpr std::ptrdiff_t operator-(int* i, const ThrowingSentinel& s) { return i - s.ptr; }
  friend constexpr std::ptrdiff_t operator-(const ThrowingSentinel& s, const ConvertibleIterator& i) {
    return s.ptr - i.ptr;
  }
  friend constexpr std::ptrdiff_t operator-(const ConvertibleIterator& i, const ThrowingSentinel& s) {
    return i.ptr - s.ptr;
  }
};

bool ThrowingSentinel::throw_on_copy = false;

using Common = std::common_iterator<int*, ThrowingSentinel>;
using OtherCommon = std::common_iterator<ConvertibleIterator, ThrowingSentinel>;

void make_valueless(Common& target, int* ptr) {
  Common source = ThrowingSentinel(ptr);
  target         = Common(ptr);
  ThrowingSentinel::throw_on_copy = true;
  bool threw                       = false;
  try {
    target = source;
  } catch (int) {
    threw = true;
  }
  ThrowingSentinel::throw_on_copy = false;
  assert(threw);
}

void make_valueless(OtherCommon& target, int* ptr) {
  OtherCommon source = ThrowingSentinel(ptr);
  target              = OtherCommon(ConvertibleIterator{ptr});
  ThrowingSentinel::throw_on_copy = true;
  bool threw                       = false;
  try {
    target = source;
  } catch (int) {
    threw = true;
  }
  ThrowingSentinel::throw_on_copy = false;
  assert(threw);
}
#endif // TEST_HAS_NO_EXCEPTIONS

int main(int, char**) {
#ifndef TEST_HAS_NO_EXCEPTIONS
  int data[] = {1, 2, 3};

  OtherCommon valueless_other = ConvertibleIterator{data};
  make_valueless(valueless_other, data);

  TEST_LIBCPP_ASSERT_FAILURE(
      Common(valueless_other), "Attempted to construct from a valueless common_iterator");

  Common valueless = data;
  make_valueless(valueless, data);

  Common valid = data;
  TEST_LIBCPP_ASSERT_FAILURE(
      valid = valueless_other, "Attempted to assign from a valueless common_iterator");

  TEST_LIBCPP_ASSERT_FAILURE(*valueless, "Attempted to dereference a non-dereferenceable common_iterator");
  const Common const_valueless = valueless;
  TEST_LIBCPP_ASSERT_FAILURE(
      *const_valueless, "Attempted to dereference a non-dereferenceable common_iterator");

  TEST_LIBCPP_ASSERT_FAILURE(valueless.operator->(),
                             "Attempted to dereference a non-dereferenceable common_iterator");

  TEST_LIBCPP_ASSERT_FAILURE(++valueless, "Attempted to increment a non-dereferenceable common_iterator");
  TEST_LIBCPP_ASSERT_FAILURE(valueless++, "Attempted to increment a non-dereferenceable common_iterator");

  TEST_LIBCPP_ASSERT_FAILURE(valueless == valid, "Attempted to compare a valueless common_iterator");
  TEST_LIBCPP_ASSERT_FAILURE(valid == valueless, "Attempted to compare a valueless common_iterator");
  TEST_LIBCPP_ASSERT_FAILURE(valueless - valid, "Attempted to subtract from a valueless common_iterator");
  TEST_LIBCPP_ASSERT_FAILURE(valid - valueless, "Attempted to subtract a valueless common_iterator");

  TEST_LIBCPP_ASSERT_FAILURE(
      std::ranges::iter_move(valueless), "Attempted to iter_move a non-dereferenceable common_iterator");
  TEST_LIBCPP_ASSERT_FAILURE(
      std::ranges::iter_swap(valueless, valid), "Attempted to iter_swap a non-dereferenceable common_iterator");
  TEST_LIBCPP_ASSERT_FAILURE(
      std::ranges::iter_swap(valid, valueless), "Attempted to iter_swap a non-dereferenceable common_iterator");
#endif // TEST_HAS_NO_EXCEPTIONS
  return 0;
}
