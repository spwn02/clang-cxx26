//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23, c++26

// <memory>

// P3125R6: std::pointer_tag_pair, std::pointer_bits_available, std::max_pointer_bits_available

#include <cassert>
#include <compare>
#include <concepts>
#include <memory>
#include <tuple>
#include <type_traits>

#include "test_macros.h"

#if !defined(__cpp_lib_pointer_tag_pair) || __cpp_lib_pointer_tag_pair != 202606L
#  error "__cpp_lib_pointer_tag_pair should be 202606L in C++29"
#endif

// --- limits ---------------------------------------------------------------------------------------------------------
static_assert(std::max_pointer_bits_available >= 1);
static_assert(std::pointer_bits_available(1) == 0);
static_assert(std::pointer_bits_available(2) == 1);
static_assert(std::pointer_bits_available(8) == 3);
static_assert(std::pointer_bits_available(alignof(int)) == 2);
static_assert(std::pointer_bits_available(size_t(1) << 40) == std::max_pointer_bits_available);

// --- types ----------------------------------------------------------------------------------------------------------
struct alignas(8) Obj {
  int value;
};
struct alignas(16) Wide {
  int value;
};

using IntPair = std::pointer_tag_pair<int*>;
static_assert(IntPair::bits_requested == 2);
static_assert(std::same_as<IntPair::pointer_type, int*>);
static_assert(std::same_as<IntPair::element_type, int>);
static_assert(std::same_as<IntPair::tagged_pointer_type, void*>);
static_assert(std::same_as<IntPair::tag_type, unsigned>);
static_assert(std::same_as<std::pointer_tag_pair<const int*>::tagged_pointer_type, const void*>);
static_assert(std::same_as<std::pointer_tag_pair<volatile int*>::tagged_pointer_type, volatile void*>);

static_assert(sizeof(IntPair) == sizeof(int*));
static_assert(alignof(IntPair) == alignof(int*));
static_assert(std::is_trivially_copyable_v<IntPair>);
static_assert(std::copyable<IntPair>);
static_assert(sizeof(std::pointer_tag_pair<Obj*, 3, unsigned char>) == sizeof(Obj*));

// Constraints on the constructors.
static_assert(std::is_constructible_v<IntPair, int*, unsigned>);
static_assert(!std::is_constructible_v<IntPair, long*, unsigned>);           // not convertible to int*
using CharPair = std::pointer_tag_pair<char*, 0>;
static_assert(CharPair::bits_requested == 0);
static_assert(std::is_constructible_v<CharPair, char*, unsigned>);
static_assert(!std::is_constructible_v<std::pointer_tag_pair<char*, 1>, char*, unsigned>); // char is not aligned to 2
static_assert(std::is_constructible_v<std::pointer_tag_pair<Obj*, 3>, Obj*, unsigned>);
static_assert(!std::is_constructible_v<std::pointer_tag_pair<Obj*, 4>, Obj*, unsigned>); // alignof(Obj) is 8

// --- constant expressions -------------------------------------------------------------------------------------------
alignas(8) constexpr int Global = 42;
constexpr Obj Objects[3] = {{1}, {2}, {3}};
constexpr Wide WideObject{7};

constexpr bool test_basic() {
  constexpr std::pointer_tag_pair<const int*> p(&Global, 2u);
  static_assert(p.pointer() == &Global);
  static_assert(p.tag() == 2u);
  static_assert(*p.pointer() == 42);

  constexpr std::pointer_tag_pair<const Obj*, 3, unsigned char> q(&Objects[2], 5);
  static_assert(q.pointer() == &Objects[2]);
  static_assert(q.pointer()->value == 3);
  static_assert(q.tag() == 5);

  constexpr std::pointer_tag_pair<const Obj*> d;
  static_assert(d.pointer() == nullptr);
  static_assert(d.tag() == 0u);

  constexpr std::pointer_tag_pair<const Obj*> n(nullptr, 7);
  static_assert(n.pointer() == nullptr);
  static_assert(n.tag() == 7u);
  return true;
}
static_assert(test_basic());

enum class Color : unsigned char { red, green, blue, yellow };
constexpr std::pointer_tag_pair<const Obj*, 2, Color> Enumerated(&Objects[1], Color::blue);
static_assert(Enumerated.pointer() == &Objects[1]);
static_assert(Enumerated.tag() == Color::blue);

// void* has no alignment of its own: the caller promises one.
constexpr auto Overaligned = std::pointer_tag_pair<const void*, 4>::from_overaligned<16>(&WideObject, 11u);
static_assert(Overaligned.pointer() == &WideObject);
static_assert(Overaligned.tag() == 11u);
static_assert(static_cast<const Wide*>(Overaligned.pointer())->value == 7);

// Pointers to a pointer-interconvertible base.
struct Base {
  int a;
};
struct alignas(8) Derived : Base {};
constexpr Derived DerivedObject{{9}};
constexpr std::pointer_tag_pair<const Base*, 2> BasePair(&DerivedObject, 3u);
static_assert(BasePair.pointer() == &DerivedObject);
static_assert(BasePair.pointer()->a == 9);
static_assert(BasePair.tag() == 3u);

// Copies, swap, comparison.
constexpr bool test_ops() {
  std::pointer_tag_pair<const Obj*> a(&Objects[0], 1);
  std::pointer_tag_pair<const Obj*> b(&Objects[0], 2);
  std::pointer_tag_pair<const Obj*> c(a);
  assert(a == c);
  assert(a != b);
  assert((a <=> b) == std::strong_ordering::less);
  assert((b <=> a) == std::strong_ordering::greater);
  assert((a <=> c) == std::strong_ordering::equal);
  a.swap(b);
  assert(a.tag() == 2 && b.tag() == 1);
  assert(a.pointer() == &Objects[0] && b.pointer() == &Objects[0]);
  std::pointer_tag_pair<const Obj*> d(&Objects[1], 0);
  assert(b != d);
  return true;
}
static_assert(test_ops());

// Tuple interface and structured bindings.
static_assert(std::tuple_size_v<IntPair> == 2);
static_assert(std::tuple_size_v<const IntPair> == 2);
static_assert(std::same_as<std::tuple_element_t<0, IntPair>, int*>);
static_assert(std::same_as<std::tuple_element_t<1, IntPair>, unsigned>);
static_assert(std::same_as<std::tuple_element_t<0, const IntPair>, int* const>);
constexpr bool test_tuple() {
  std::pointer_tag_pair<const Obj*> p(&Objects[1], 3);
  auto [ptr, tag] = p;
  assert(ptr == &Objects[1]);
  assert(tag == 3);
  assert(std::get<0>(p) == &Objects[1]);
  assert(std::get<1>(p) == 3);
  return true;
}
static_assert(test_tuple());

// Class template argument deduction.
constexpr bool test_ctad() {
  int local = 5;
  std::pointer_tag_pair p(&local, 1u);
  static_assert(std::same_as<decltype(p), std::pointer_tag_pair<int*, 2, unsigned>>);
  return true;
}

// --- run time -------------------------------------------------------------------------------------------------------
int main(int, char**) {
  // heap objects
  Obj* o = new Obj{4};
  std::pointer_tag_pair<Obj*, 3> tagged(o, 6);
  assert(tagged.pointer() == o);
  assert(tagged.tag() == 6);
  tagged.pointer()->value = 5;
  assert(o->value == 5);

  // the raw tagged pointer round trips and is not the plain pointer
  void* raw = tagged.tagged_pointer();
  assert(raw != static_cast<void*>(o));
  auto back = std::pointer_tag_pair<Obj*, 3>::from_tagged(raw);
  assert(back == tagged);
  assert(back.pointer() == o);
  assert(back.tag() == 6);

  // other tag values of the same pointer
  for (unsigned t = 0; t < 8; ++t) {
    std::pointer_tag_pair<Obj*, 3> p(o, t);
    assert(p.tag() == t);
    assert(p.pointer() == o);
  }

  // nullptr
  std::pointer_tag_pair<Obj*, 3> n(nullptr, 5);
  assert(n.pointer() == nullptr);
  assert(n.tag() == 5);

  // storing the raw pointer in an atomic-sized slot
  static_assert(sizeof(std::pointer_tag_pair<Obj*, 3>) == sizeof(void*));

  // from_overaligned on an over-aligned heap object
  struct alignas(32) Big {
    char c;
  };
  Big* big = new Big{};
  auto op  = std::pointer_tag_pair<Big*, 5>::from_overaligned<32>(big, 31);
  assert(op.pointer() == big);
  assert(op.tag() == 31);
  delete big;
  delete o;
  test_ctad();
  return 0;
}
