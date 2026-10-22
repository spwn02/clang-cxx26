//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// members_of(r) for a namespace returns the members declared in every block of the
// namespace ([meta.reflection.member.queries]), including
//  - members of the blocks that precede a block whose first declaration belongs to another scope, and
//  - an entity whose first declaration is a friend declaration (not members-of-eligible).

#include <meta>

namespace N {
inline namespace I {
struct A {
  void f();
};
} // namespace I
} // namespace N

// The first declaration of this block is an out-of-line member function definition: its
// semantic context is the class A, not the namespace.
namespace N {
inline namespace I {
void A::f() {}
struct B {};
} // namespace I
} // namespace N

namespace N {
inline namespace I {
struct Owner {
  friend class Friended;
};
} // namespace I
} // namespace N

namespace N {
inline namespace I {
class Friended {};
struct C {};
} // namespace I
} // namespace N

// A friend declaration is not members-of-eligible: neither a class that is only declared as a friend nor a hidden
// friend function is a member of the namespace.
namespace N {
inline namespace I {
struct Owner2 {
  friend class OnlyFriend;
  friend void hidden_friend(Owner2) {}
};
} // namespace I
} // namespace N

consteval bool has(std::meta::info ns, std::string_view name) {
  for (auto m : std::meta::members_of(ns, std::meta::access_context::unchecked()))
    if (std::meta::has_identifier(m) && std::meta::identifier_of(m) == name)
      return true;
  return false;
}

static_assert(has(^^N::I, "A"));
static_assert(has(^^N::I, "B"));
static_assert(has(^^N::I, "C"));
static_assert(has(^^N::I, "Owner"));
static_assert(has(^^N::I, "Friended"));

static_assert(has(^^N::I, "Owner2"));
static_assert(!has(^^N::I, "OnlyFriend"));
static_assert(!has(^^N::I, "hidden_friend"));

consteval int count(std::meta::info ns, std::string_view name) {
  int n = 0;
  for (auto m : std::meta::members_of(ns, std::meta::access_context::unchecked()))
    if (std::meta::has_identifier(m) && std::meta::identifier_of(m) == name)
      ++n;
  return n;
}
static_assert(count(^^N::I, "Friended") == 1);

int main(int, char**) { return 0; }
