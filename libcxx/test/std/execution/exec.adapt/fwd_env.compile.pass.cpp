//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <execution>

// LWG4203: a sender adaptor evaluates its child sender against FWD-ENV(env), so a query
// that is not a forwarding query is not visible to the child.

#include <execution>
#include <optional>
namespace ex = std::execution;
struct q_t {
  template<class E> constexpr auto operator()(const E& e) const noexcept -> decltype(e.query(*this)) { return e.query(*this); }
};
inline constexpr q_t q{};                       // not a forwarding query
struct Env { constexpr int query(q_t) const noexcept { return 1; } };
template<class S> concept ok = ex::sender_in<S, Env>;
using R = decltype(ex::read_env(q));
static_assert(ok<R>);                                                    // direct: visible
static_assert(!ok<decltype(ex::then(R{}, [](int){}))>);                  // control: then forwards only forwarding queries
static_assert(!ok<decltype(ex::when_all(R{}))>);
static_assert(!ok<decltype(ex::let_value(R{}, [](int){ return ex::just(); }))>);
static_assert(!ok<decltype(ex::continues_on(R{}, ex::inline_scheduler{}))>);
static_assert(!ok<decltype(ex::into_variant(R{}))>);
static_assert(!ok<decltype(ex::stopped_as_optional(R{}))>);
static_assert(!ok<decltype(ex::stopped_as_error(R{}, 1))>);
static_assert(!ok<decltype(ex::upon_error(R{}, [](auto){ return 0; }))>);
