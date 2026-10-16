//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <execution>

// [exec.getcomplsigs]: get_completion_signatures<Sndr, Env...>() is a consteval function that
//   - evaluates CHECKED-COMPLSIGS(get-complsigs<NewSndr, Env...>()) if that expression is well-formed (even if its
//     result is not a valid completion_signatures: then it throws an exception derived from `exception`),
//   - otherwise CHECKED-COMPLSIGS(get-complsigs<NewSndr>()) if that is well-formed,
//   - otherwise the signatures of an awaitable sender, if NewSndr is awaitable,
//   - otherwise throws dependent_sender_error if there is no environment,
//   - otherwise throws an exception derived from `exception` (but not from dependent_sender_error).
// [exec.snd.concepts]: sender_in requires the call to be a constant expression; dependent_sender asks whether the
// call without an environment throws dependent_sender_error.

#include <cassert>
#include <concepts>
#include <coroutine>
#include <exception>
#include <execution>
#include <type_traits>

namespace ex = std::execution;

using CS = ex::completion_signatures<ex::set_value_t()>;

// ---------------------------------------------------------------------------------------------------------------
// no signatures at all, with and without an environment
struct NoSignatures {
  using sender_concept = ex::sender_tag;
};

consteval bool dependent_without_env() {
  try {
    ex::get_completion_signatures<NoSignatures>();
    return false;
  } catch (ex::dependent_sender_error&) {
    return true;
  }
}
static_assert(dependent_without_env());
static_assert(ex::dependent_sender<NoSignatures>);
static_assert(!ex::sender_in<NoSignatures>);

consteval int other_exception_with_env() {
  try {
    ex::get_completion_signatures<NoSignatures, ex::env<>>();
    return 0;
  } catch (ex::dependent_sender_error&) {
    return 1;
  } catch (std::exception&) {
    return 2;
  }
}
static_assert(other_exception_with_env() == 2);
static_assert(!ex::sender_in<NoSignatures, ex::env<>>);

// a sender with signatures that do not depend on the environment is not dependent
struct Plain {
  using sender_concept = ex::sender_tag;
  ex::env<> get_env() const noexcept { return {}; }
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return CS{};
  }
};
static_assert(ex::sender_in<Plain>);
static_assert(ex::sender_in<Plain, ex::env<>>);
static_assert(!ex::dependent_sender<Plain>);
static_assert(std::same_as<ex::completion_signatures_of_t<Plain>, CS>);

// a sender with signatures that need an environment
struct NeedsEnv {
  using sender_concept = ex::sender_tag;
  ex::env<> get_env() const noexcept { return {}; }
  template <class Self, class Env>
  static consteval auto get_completion_signatures() {
    return CS{};
  }
};
static_assert(ex::dependent_sender<NeedsEnv>);
static_assert(!ex::sender_in<NeedsEnv>);
static_assert(ex::sender_in<NeedsEnv, ex::env<>>);
static_assert(std::same_as<ex::completion_signatures_of_t<NeedsEnv, ex::env<>>, CS>);

// something that is not a sender is not a dependent sender
static_assert(!ex::dependent_sender<int>);

// ---------------------------------------------------------------------------------------------------------------
// the first well-formed member expression is the one that counts, whatever it returns
struct InvalidFirst {
  using sender_concept = ex::sender_tag;
  ex::env<> get_env() const noexcept { return {}; }
  template <class Self, class Env>
  static consteval int get_completion_signatures() {
    return 0;
  }
  template <class Self>
  static consteval auto get_completion_signatures() {
    return CS{};
  }
};
consteval bool invalid_first_throws() {
  try {
    (void)ex::get_completion_signatures<InvalidFirst, ex::env<>>();
    return false;
  } catch (std::exception&) {
    return true;
  }
}
static_assert(invalid_first_throws());
static_assert(!ex::sender_in<InvalidFirst, ex::env<>>);
// without an environment only the second one is a candidate
static_assert(ex::sender_in<InvalidFirst>);

// the environment-less overload is used when there is no overload taking one
struct OnlyNoEnv {
  using sender_concept = ex::sender_tag;
  ex::env<> get_env() const noexcept { return {}; }
  template <class Self>
  static consteval auto get_completion_signatures() {
    return CS{};
  }
};
static_assert(std::same_as<ex::completion_signatures_of_t<OnlyNoEnv, ex::env<>>, CS>);
static_assert(ex::sender_in<OnlyNoEnv>);

// a result that is not completion_signatures
struct NotSignatures {
  using sender_concept = ex::sender_tag;
  ex::env<> get_env() const noexcept { return {}; }
  template <class Self, class... Env>
  static consteval int get_completion_signatures() {
    return 0;
  }
};
consteval bool not_signatures_throws() {
  try {
    (void)ex::get_completion_signatures<NotSignatures>();
    return false;
  } catch (std::exception&) {
    return true;
  }
}
static_assert(not_signatures_throws());
static_assert(!ex::sender_in<NotSignatures>);
static_assert(!ex::dependent_sender<NotSignatures>); // an unspecified exception, not dependent_sender_error

// ---------------------------------------------------------------------------------------------------------------
// sender_in: the call must be a constant expression
struct NotConstexpr {
  using sender_concept = ex::sender_tag;
  ex::env<> get_env() const noexcept { return {}; }
  template <class Self, class... Env>
  static auto get_completion_signatures() {
    return CS{};
  }
};
static_assert(!ex::sender_in<NotConstexpr>);
static_assert(!ex::sender_in<NotConstexpr, ex::env<>>);

// ---------------------------------------------------------------------------------------------------------------
// awaitables are senders
struct Awaitable {
  bool await_ready() noexcept { return true; }
  void await_suspend(std::coroutine_handle<>) noexcept {}
  int await_resume() { return 1; }
};
static_assert(ex::sender_in<Awaitable>);
static_assert(ex::sender_in<Awaitable, ex::env<>>);
static_assert(!ex::dependent_sender<Awaitable>);
static_assert(std::same_as<ex::completion_signatures_of_t<Awaitable>,
                           ex::completion_signatures<ex::set_value_t(int), ex::set_error_t(std::exception_ptr),
                                                     ex::set_stopped_t()>>);

// at most one environment
static_assert(!ex::sender_in<Plain, ex::env<>, ex::env<>>);

int main(int, char**) { return 0; }
