//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: no-threads
// UNSUPPORTED: libcpp-has-no-incomplete-pstl


// <execution>

// [exec.when.all]p9 check-types: a child with two or more set_value completions is an error (get_completion_signatures
// throws, so the sender is not a sender_in), and so is a child whose result datums are not decay-copyable; a child with
// no set_value completion is fine. when_all_with_variant is not affected (into_variant makes a two-value child one value).

#include <execution>

namespace ex = std::execution;

struct Child {
  using sender_concept = ex::sender_tag;
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(int), ex::set_value_t(double)>{};
  }
};
static_assert(ex::sender_in<Child, ex::env<>>);
static_assert(!ex::sender_in<decltype(ex::when_all(Child{})), ex::env<>>);
static_assert(!ex::sender_in<decltype(ex::when_all(ex::just(1), Child{})), ex::env<>>);
static_assert(ex::sender_in<decltype(ex::when_all(ex::just(1), ex::just(2.0))), ex::env<>>);
static_assert(ex::sender_in<decltype(ex::when_all_with_variant(Child{})), ex::env<>>);
static_assert(ex::sender_in<decltype(ex::when_all_with_variant(ex::just(1), Child{})), ex::env<>>);

struct NoCopy {
  NoCopy()              = default;
  NoCopy(const NoCopy&) = delete;
  NoCopy(NoCopy&&)      = delete;
};
struct NoCopyChild {
  using sender_concept = ex::sender_tag;
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(NoCopy&&)>{};
  }
};
static_assert(ex::sender_in<NoCopyChild, ex::env<>>);
static_assert(!ex::sender_in<decltype(ex::when_all(NoCopyChild{})), ex::env<>>);
static_assert(!ex::sender_in<decltype(ex::into_variant(NoCopyChild{})), ex::env<>>);
static_assert(ex::sender_in<decltype(ex::when_all(ex::just_stopped(), ex::just(1))), ex::env<>>);
static_assert(ex::sender_in<decltype(ex::when_all(ex::just_error(1))), ex::env<>>);

int main(int, char**) { return 0; }
