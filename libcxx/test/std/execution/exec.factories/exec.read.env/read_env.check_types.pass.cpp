//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <execution>

// [exec.read.env]: read_env(q) is a dependent sender (its signatures need the environment). With an environment its
// completion signatures are those of the value of the query, and a query that cannot be called with the environment,
// or whose result is not an object that can complete with, makes the signatures an unspecified exception (which
// derives from exception, but not from dependent_sender_error).

#include <cassert>
#include <concepts>
#include <exception>
#include <execution>

namespace ex = std::execution;

struct GoodQuery {
  template <class Env>
  int operator()(const Env&) const noexcept {
    return 1;
  }
};
struct VoidQuery {
  template <class Env>
  void operator()(const Env&) const noexcept {}
};
struct NotCallable {};

using GoodSender = decltype(ex::read_env(GoodQuery{}));
using VoidSender = decltype(ex::read_env(VoidQuery{}));

static_assert(ex::dependent_sender<GoodSender>);
static_assert(!ex::sender_in<GoodSender>);
static_assert(ex::sender_in<GoodSender, ex::env<>>);
static_assert(std::same_as<ex::completion_signatures_of_t<GoodSender, ex::env<>>,
                           ex::completion_signatures<ex::set_value_t(int)>>);

// the exception kinds: none, dependent_sender_error, another exception
template <class Sndr>
consteval int kind() {
  try {
    ex::get_completion_signatures<Sndr, ex::env<>>();
    return 0;
  } catch (ex::dependent_sender_error&) {
    return 1;
  } catch (std::exception&) {
    return 2;
  }
}

// a query whose result is void is not valid either: Q()(env) must have a type that is not void
static_assert(kind<VoidSender>() == 2);
static_assert(!ex::sender_in<VoidSender, ex::env<>>);
static_assert(kind<GoodSender>() == 0);
static_assert(kind<decltype(ex::read_env(NotCallable{}))>() == 2);
// without an environment the exception is the one of a dependent sender
consteval int kind_without_env() {
  try {
    ex::get_completion_signatures<GoodSender>();
    return 0;
  } catch (ex::dependent_sender_error&) {
    return 1;
  } catch (std::exception&) {
    return 2;
  }
}
static_assert(kind_without_env() == 1);

struct NotCallableQuery {
  void operator()(int) const noexcept {}
};
static_assert(kind<decltype(ex::read_env(NotCallableQuery{}))>() == 2);
static_assert(!ex::sender_in<decltype(ex::read_env(NotCallableQuery{})), ex::env<>>);

// a query that may throw is a sender with an error completion, whose environment-less signatures are an error
struct ThrowingQuery {
  template <class Env>
  int operator()(const Env&) const {
    return 1;
  }
};
static_assert(ex::sender_in<decltype(ex::read_env(ThrowingQuery{})), ex::env<>>);

int main(int, char**) { return 0; }
