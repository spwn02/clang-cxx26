//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

#include <type_traits>

struct Public { int i; };
class Private { [[maybe_unused]] int i; };
struct Mutable { mutable int i; };
struct NonLiteral { ~NonLiteral(); };
union PublicUnion { int i; float f; };
union PrivateUnion { private: [[maybe_unused]] int i; };
struct PrivateBase : private Public {};
struct Incomplete;
template<class T> struct Wrapper { T value; };

template<class T, bool Expected>
constexpr bool check() {
  static_assert(std::is_structural<T>::value == Expected);
  static_assert(std::is_structural<const T>::value == Expected);
  static_assert(std::is_structural<volatile T>::value == Expected);
  static_assert(std::is_structural<const volatile T>::value == Expected);
  static_assert(std::is_structural_v<T> == Expected);
  static_assert(std::is_structural_v<const T> == Expected);
  static_assert(std::is_structural_v<volatile T> == Expected);
  static_assert(std::is_structural_v<const volatile T> == Expected);
  static_assert(std::is_base_of_v<std::bool_constant<Expected>, std::is_structural<T>>);
  return true;
}

static_assert(check<int, true>());
static_assert(check<int*, true>());
static_assert(check<int&, true>());
static_assert(check<Incomplete&, true>());
static_assert(check<int&&, false>());
static_assert(check<void, false>());
static_assert(check<void(), false>());
static_assert(check<int[2], false>());
static_assert(check<int[], false>());
static_assert(check<Public[2], false>());
static_assert(check<Public, true>());
static_assert(check<PublicUnion, true>());
static_assert(check<Private, false>());
static_assert(check<Mutable, false>());
static_assert(check<NonLiteral, false>());
static_assert(check<PrivateUnion, false>());
static_assert(check<PrivateBase, false>());
static_assert(check<Wrapper<Public>, true>());
static_assert(check<Wrapper<Private>, false>());

int main(int, char**) { return 0; }
