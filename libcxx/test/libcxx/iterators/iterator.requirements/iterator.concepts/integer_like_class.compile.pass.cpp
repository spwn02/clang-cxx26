//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17

// P2393R1: the integer-like concepts model integer-class types, not only the built-in integral types.

#include <iterator>

#include <concepts>
#include <limits>
#include <type_traits>

#include "integer_class.h"
#include "test_macros.h"

#ifdef __SIZEOF_INT128__

using S = integer_class::Signed;
using U = integer_class::Unsigned;

static_assert(std::numeric_limits<S>::is_integer && std::numeric_limits<S>::is_signed);
static_assert(std::numeric_limits<U>::is_integer && !std::numeric_limits<U>::is_signed);
static_assert(!std::integral<S> && !std::integral<U>);

static_assert(std::__integer_class<S>);
static_assert(std::__integer_class<U>);
static_assert(!std::__integer_class<int>);
static_assert(!std::__integer_class<bool>);
static_assert(!std::__integer_class<float>);
static_assert(!std::__integer_class<void>);
static_assert(!std::__integer_class<std::byte>);
enum E { e };
static_assert(!std::__integer_class<E>);
struct NotInteger {};
static_assert(!std::__integer_class<NotInteger>);

static_assert(std::__integer_like<S>);
static_assert(std::__integer_like<U>);
static_assert(std::__signed_integer_like<S>);
static_assert(!std::__signed_integer_like<U>);
static_assert(std::__unsigned_integer_like<U>);
static_assert(!std::__unsigned_integer_like<S>);

// The built-in types are unchanged.
static_assert(std::__integer_like<int> && std::__signed_integer_like<int> && !std::__unsigned_integer_like<int>);
static_assert(std::__unsigned_integer_like<unsigned> && !std::__signed_integer_like<unsigned>);
static_assert(!std::__integer_like<bool> && !std::__unsigned_integer_like<bool>);
static_assert(!std::__integer_like<NotInteger> && !std::__signed_integer_like<NotInteger>);

// make-signed-like-t / make-unsigned-like-t
static_assert(std::same_as<std::__make_unsigned_like_t<S>, U>);
static_assert(std::same_as<std::__make_unsigned_like_t<U>, U>);
static_assert(std::same_as<std::__make_signed_like_t<U>, S>);
static_assert(std::same_as<std::__make_signed_like_t<S>, S>);
static_assert(std::same_as<std::__make_unsigned_like_t<int>, unsigned>);
static_assert(std::same_as<std::__make_signed_like_t<unsigned long>, long>);
static_assert(std::__to_unsigned_like(S(-1)) == ~U(0));
static_assert(std::__to_unsigned_like(-1) == ~0u);

// A weakly_incrementable type whose difference type is an integer-class type.
struct CountIt {
  using difference_type = S;
  int value = 0;
  CountIt& operator++() { ++value; return *this; }
  CountIt operator++(int) { auto t = *this; ++value; return t; }
  friend bool operator==(CountIt, CountIt) = default;
};
static_assert(std::weakly_incrementable<CountIt>);
static_assert(std::incrementable<CountIt>);
static_assert(std::same_as<std::iter_difference_t<CountIt>, S>);

struct UnsignedDiffIt {
  using difference_type = U;
  UnsignedDiffIt& operator++();
  UnsignedDiffIt operator++(int);
};
static_assert(!std::weakly_incrementable<UnsignedDiffIt>); // the difference type must be signed

#endif // __SIZEOF_INT128__
