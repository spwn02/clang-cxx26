//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <execution>

// struct forwarding_query_t {
//   template<class Query>
//   constexpr bool operator()(const Query&) const noexcept;
// };
// inline constexpr forwarding_query_t forwarding_query{};

#include <cassert>
#include <execution>

// Opts in via inheritance from forwarding_query_t.
struct ForwardingByInheritance : std::forwarding_query_t {};

// Opts in (and can override) via a member `query(forwarding_query_t)`.
struct ForwardsTrueByMember {
  constexpr bool query(std::forwarding_query_t) const noexcept { return true; }
};

struct ForwardsFalseByMember {
  constexpr bool query(std::forwarding_query_t) const noexcept { return false; }
};

// The member form takes precedence over inheritance.
struct OverridesInheritance : std::forwarding_query_t {
  constexpr bool query(std::forwarding_query_t) const noexcept { return false; }
};

struct NotForwarding {};

constexpr bool test() {
  using namespace std;

  assert(forwarding_query(ForwardingByInheritance{}));
  assert(forwarding_query(ForwardsTrueByMember{}));
  assert(!forwarding_query(ForwardsFalseByMember{}));
  assert(!forwarding_query(OverridesInheritance{}));
  assert(!forwarding_query(NotForwarding{}));

  static_assert(noexcept(forwarding_query(NotForwarding{})));

  return true;
}

int main(int, char**) {
  test();
  static_assert(test());

  return 0;
}
