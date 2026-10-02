// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20 || c++23
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

// [meta.reflection.access.queries] is_accessible checks designating-class
// membership before the null-scope shortcut; access_context is a structural,
// non-aggregate, assignable struct; has_inaccessible_subobjects (#187).

#include <meta>
#include <type_traits>

using namespace std::meta;

struct Other { static constexpr int o = 1; };
struct D1 {};
struct P { int x; };
struct Q : P {};

// A class member that is not a member of the designating class is inaccessible
// even with an unchecked (null-scope) context.
static_assert(!is_accessible(^^Other::o, access_context::unchecked().via(^^D1)));
static_assert(is_accessible(^^Other::o, access_context::unchecked().via(^^Other)));
static_assert(is_accessible(^^P::x, access_context::unchecked().via(^^Q)));  // inherited member
static_assert(!is_accessible(^^P::x, access_context::unchecked().via(^^D1)));
static_assert(!is_accessible(bases_of(^^Q, access_context::unchecked())[0],
                             access_context::unchecked().via(^^D1)));
static_assert(is_accessible(bases_of(^^Q, access_context::unchecked())[0],
                            access_context::unchecked().via(^^Q)));

static_assert(!std::is_aggregate_v<access_context>);
static_assert(std::is_class_v<access_context> && std::is_copy_assignable_v<access_context>);
consteval bool assignable() {
  auto a = access_context::unchecked();
  auto b = access_context::unprivileged();
  a = b;
  return a.scope() == ^^::;
}
static_assert(assignable());

struct Mixed { private: [[maybe_unused]] int pr; public: int pu; };
struct AllPublic { int a; };
static_assert(has_inaccessible_subobjects(^^Mixed, access_context::unprivileged()));
static_assert(!has_inaccessible_subobjects(^^AllPublic, access_context::unprivileged()));

int main(int, char**) { return 0; }
