//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <execution>

// template<queryable... Envs>
// class env {
// public:
//   constexpr env() = default;
//   template<class... Vs> constexpr explicit env(Vs&&...);
//   template<class Query, class... Args> constexpr decltype(auto) query(Query, Args&&...) const;
// };
// template<class... Envs>
// env(Envs...) -> env<unwrap_reference_t<Envs>...>;

#include <cassert>
#include <execution>
#include <type_traits>
#include <utility>

struct QueryA {};
struct QueryB {};
struct QueryC {};

// A `requires{ e.query(q); }` written with `e`/`q` naming concrete (non-dependent) entities
// is evaluated eagerly, not as a substitution-failure-is-fine probe: an invalid expression
// there is ill-formed, full stop, rather than making the requires-expression `false` (that
// "soft" behavior only applies when a template parameter is actually being substituted, e.g.
// inside `Env`/`Query` below). Route every "this query is unsupported" check through this
// concept instead of writing `requires{}` directly against concrete local variables.
template <class Env, class Query>
concept CanQuery = requires(const Env& e, Query q) { e.query(q); };

constexpr bool test() {
  using namespace std::execution;

  // env<> supports no queries at all
  static_assert(!CanQuery<env<>, QueryA>);

  {
    // a single-element env forwards to that element
    env<prop<QueryA, int>> e{prop<QueryA, int>{QueryA{}, 1}};
    assert(e.query(QueryA{}) == 1);
    static_assert(!CanQuery<decltype(e), QueryB>);
  }

  {
    // dispatches to the first matching element when several are present
    env<prop<QueryA, int>, prop<QueryB, int>> e{prop<QueryA, int>{QueryA{}, 1}, prop<QueryB, int>{QueryB{}, 2}};
    assert(e.query(QueryA{}) == 1);
    assert(e.query(QueryB{}) == 2);
    static_assert(!CanQuery<decltype(e), QueryC>);
  }

  {
    // CanQuery<env<prop<QueryA, int>>, QueryB> must itself stay well-formed (evaluate to
    // `false`) rather than recursing past the end of the Envs pack: this exercises the
    // `_Idx == sizeof...(_Envs)` guard in `__query_is_noexcept`, which exists precisely
    // because forming `query()`'s type (its noexcept-specifier) is not SFINAE-protected the
    // way its trailing requires-clause is.
    static_assert(!CanQuery<env<prop<QueryA, int>>, QueryB>);
  }

  {
    // env<...> nests: an outer env with no match for a query falls through to an inner env
    env<prop<QueryA, int>> __inner{prop<QueryA, int>{QueryA{}, 42}};
    env<prop<QueryB, int>, env<prop<QueryA, int>>> __outer{prop<QueryB, int>{QueryB{}, 7}, __inner};
    assert(__outer.query(QueryA{}) == 42);
    assert(__outer.query(QueryB{}) == 7);
  }

  // CTAD
  {
    auto e = env(prop<QueryA, int>{QueryA{}, 1}, prop<QueryB, int>{QueryB{}, 2});
    static_assert(std::is_same_v<decltype(e), env<prop<QueryA, int>, prop<QueryB, int>>>);
  }

  return true;
}

int main(int, char**) {
  test();
  static_assert(test());

  return 0;
}
