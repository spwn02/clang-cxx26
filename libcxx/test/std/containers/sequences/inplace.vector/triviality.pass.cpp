//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <inplace_vector>

// [containers.sequences.inplace.vector.overview]p3: the destructor, copy/move
// constructors, and copy/move assignment operators are each individually
// trivial according to T's corresponding triviality (with copy/move
// assignment additionally requiring T to be trivially destructible); if N is
// zero the whole container is trivial regardless of T.

#include <inplace_vector>
#include <type_traits>

#include "test_macros.h"

struct TrivialDtorOnly {
  TrivialDtorOnly()                                   {}
  TrivialDtorOnly(const TrivialDtorOnly&)              {}
  TrivialDtorOnly(TrivialDtorOnly&&)                   {}
  TrivialDtorOnly& operator=(const TrivialDtorOnly&) = default;
  TrivialDtorOnly& operator=(TrivialDtorOnly&&)      = default;
  ~TrivialDtorOnly()                                 = default;
};
static_assert(std::is_trivially_destructible_v<TrivialDtorOnly>);
static_assert(!std::is_trivially_copy_constructible_v<TrivialDtorOnly>);
static_assert(!std::is_trivially_move_constructible_v<TrivialDtorOnly>);

struct TrivialCopyOnly {
  int x = 0;
  TrivialCopyOnly()                                   = default;
  TrivialCopyOnly(const TrivialCopyOnly&)             = default;
  TrivialCopyOnly(TrivialCopyOnly&& o) : x(o.x) { o.x = -1; }
  TrivialCopyOnly& operator=(const TrivialCopyOnly&)  = default;
  TrivialCopyOnly& operator=(TrivialCopyOnly&&)       = default;
  ~TrivialCopyOnly()                                  = default;
};
static_assert(std::is_trivially_copy_constructible_v<TrivialCopyOnly>);
static_assert(!std::is_trivially_move_constructible_v<TrivialCopyOnly>);
static_assert(std::is_trivially_destructible_v<TrivialCopyOnly>);

// Fully trivial T: every special member of inplace_vector is trivial too.
static_assert(std::is_trivially_copyable_v<std::inplace_vector<int, 4>>);
static_assert(std::is_trivially_destructible_v<std::inplace_vector<int, 4>>);
static_assert(std::is_trivially_copy_constructible_v<std::inplace_vector<int, 4>>);
static_assert(std::is_trivially_move_constructible_v<std::inplace_vector<int, 4>>);
static_assert(std::is_trivially_copy_assignable_v<std::inplace_vector<int, 4>>);
static_assert(std::is_trivially_move_assignable_v<std::inplace_vector<int, 4>>);

// N == 0: trivial regardless of T, even a T with no trivial special members at all.
static_assert(std::is_trivially_copyable_v<std::inplace_vector<TrivialDtorOnly, 0>> &&
              std::is_trivially_default_constructible_v<std::inplace_vector<TrivialDtorOnly, 0>>);
static_assert(std::is_trivially_destructible_v<std::inplace_vector<TrivialDtorOnly, 0>>);
static_assert(std::is_trivially_copy_constructible_v<std::inplace_vector<TrivialDtorOnly, 0>>);
static_assert(std::is_trivially_move_constructible_v<std::inplace_vector<TrivialDtorOnly, 0>>);

// Destructor triviality tracks is_trivially_destructible_v<T> independently
// of copy/move-constructor triviality.
static_assert(std::is_trivially_destructible_v<std::inplace_vector<TrivialDtorOnly, 4>>);
static_assert(!std::is_trivially_copy_constructible_v<std::inplace_vector<TrivialDtorOnly, 4>>);
static_assert(!std::is_trivially_move_constructible_v<std::inplace_vector<TrivialDtorOnly, 4>>);

// Copy-constructor triviality is independent of move-constructor triviality.
static_assert(std::is_trivially_copy_constructible_v<std::inplace_vector<TrivialCopyOnly, 4>>);
static_assert(!std::is_trivially_move_constructible_v<std::inplace_vector<TrivialCopyOnly, 4>>);
static_assert(std::is_trivially_destructible_v<std::inplace_vector<TrivialCopyOnly, 4>>);

int main(int, char**) { return 0; }
