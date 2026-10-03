//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20 || c++23
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

#include <meta>

// [expr.const.reflect]: the constexpr initializer's context excludes the
// injection sequenced before the recursive call; the ordinary query sees it.
struct S;
consteval std::size_t f(int p) {
  constexpr std::size_t r =
    std::meta::is_complete_type(^^S) ? 1 : 2;   // #1
  if (!std::meta::is_complete_type(^^S)) {      // #2
    std::meta::define_aggregate(^^S, {});
  }
  return (p > 0) ? f(p - 1) : r;
}

consteval {
  if (f(1) != 2) {
    throw;                                      // OK, not evaluated
  }
}

namespace constexpr_local {
struct S;
consteval bool test(int p) {
  constexpr bool complete = std::meta::is_complete_type(^^S);
  if (p) {
    std::meta::define_aggregate(^^S, {});
    return test(0);
  }
  return !complete && std::meta::is_complete_type(^^S) &&
         std::meta::nonstatic_data_members_of(^^S, std::meta::access_context::unchecked()).empty();
}
consteval { if (!test(1)) throw; }
}

namespace templated {
struct S;
template <std::meta::info R>
consteval bool test(int p) {
  constexpr bool complete = std::meta::is_complete_type(R);
  if (p) {
    std::meta::define_aggregate(R, {});
    return test<R>(0);
  }
  return !complete && std::meta::is_complete_type(R);
}
consteval { if (!test<^^S>(1)) throw; }
}

// A call inside a consteval function is not an immediate invocation: it stays
// in the enclosing context. An invocation in a constexpr initializer is nested.
namespace immediate {
struct S;
consteval bool query() { return std::meta::is_complete_type(^^S); }
consteval bool test(int p) {
  constexpr bool complete = query();
  if (p) {
    std::meta::define_aggregate(^^S, {});
    return test(0);
  }
  return !complete && query();
}
consteval { if (!test(1)) throw; }
}

namespace immediate_invocation {
struct S;
consteval bool query() { return std::meta::is_complete_type(^^S); }
constexpr bool nested() { return query(); } // immediate invocation
consteval {
  std::meta::define_aggregate(^^S, {});
  if (nested() || !query()) throw;
}
}

namespace members {
struct S;
consteval bool test(int p) {
  constexpr bool hidden = [] consteval {
    try {
      (void)std::meta::members_of(^^S, std::meta::access_context::unchecked());
      return false;
    } catch (const std::meta::exception&) {
      return true;
    }
  }();
  constexpr bool enumerable = std::meta::is_enumerable_type(^^S);
  if (p) {
    std::meta::define_aggregate(^^S, {std::meta::data_member_spec(^^int, {.name = "x"})});
    return test(0);
  }
  return hidden && !enumerable && std::meta::is_enumerable_type(^^S) &&
         std::meta::nonstatic_data_members_of(^^S, std::meta::access_context::unchecked()).size() == 1;
}
consteval { if (!test(1)) throw; }
}

int main(int, char**) {}
