//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17

// <bitset>

// template<class charT>
//   explicit bitset(const charT* str, ...);
//
// LWG4294: the constructor does not participate in overload resolution
// if charT is an array type.

#include <bitset>
#include <type_traits>

template <class CharT>
concept can_construct = requires(const CharT* p) { std::bitset<4>(p); };

struct NotChar {
  NotChar(const NotChar&) {}
};

static_assert(can_construct<char>);
static_assert(can_construct<wchar_t>);
static_assert(!can_construct<int[2]>);
static_assert(!can_construct<NotChar>);
