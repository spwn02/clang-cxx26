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

// P3388R3 / [exec.snd.general]: the exception specification of the connect of a library sender says whether the
// operation state can be constructed without throwing: it is noexcept when the data of the sender is nothrow
// copyable/movable and, for an adaptor, the child can be connected without throwing.

#include <execution>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

namespace ex = std::execution;

struct NothrowRcvr {
  using receiver_concept = ex::receiver_tag;
  template <class... Ts>
  void set_value(Ts&&...) && noexcept {}
  template <class E>
  void set_error(E&&) && noexcept {}
  void set_stopped() && noexcept {}
};

template <class Sndr>
constexpr bool connect_nothrow = noexcept(ex::connect(std::declval<Sndr>(), NothrowRcvr{}));

// factories: the datums are decay-copied
static_assert(connect_nothrow<decltype(ex::just(1))>);
static_assert(connect_nothrow<decltype(ex::just(1, 2.0))>);
static_assert(connect_nothrow<decltype(ex::just_error(1))>);
static_assert(connect_nothrow<decltype(ex::just_stopped())>);
static_assert(connect_nothrow<decltype(ex::just(std::make_unique<int>(1)))>); // a move
static_assert(connect_nothrow<decltype(ex::just(std::string("x")))>);          // a move of a string
static_assert(connect_nothrow<decltype(ex::read_env(std::get_stop_token))>);

// lvalues are copied: only nothrow if the copy is
struct ThrowingCopy {
  ThrowingCopy() = default;
  ThrowingCopy(const ThrowingCopy&) noexcept(false) {}
  ThrowingCopy(ThrowingCopy&&) noexcept = default;
};
static_assert(connect_nothrow<decltype(ex::just(ThrowingCopy{}))>);
static_assert(!noexcept(ex::connect(std::declval<decltype(ex::just(ThrowingCopy{}))&>(), NothrowRcvr{})));
static_assert(noexcept(ex::connect(std::declval<decltype(ex::just(1))&>(), NothrowRcvr{})));

// adaptors of nothrow children and nothrow data
inline constexpr auto to_int = [](int x) noexcept { return x; };
static_assert(connect_nothrow<decltype(ex::then(ex::just(1), to_int))>);
static_assert(connect_nothrow<decltype(ex::upon_error(ex::just_error(1), to_int))>);
static_assert(connect_nothrow<decltype(ex::schedule_from(ex::just(1)))>);
static_assert(connect_nothrow<decltype(ex::write_env(ex::just(1), ex::env<>{}))>);
static_assert(connect_nothrow<decltype(ex::into_variant(ex::just(1)))>);

// The let family and everything that is lowered to it or connects further operation states internally is deliberately
// left potentially throwing (the exception specification is unspecified for these; this documents the current choice).
static_assert(!connect_nothrow<decltype(ex::stopped_as_optional(ex::just(1)))>);
static_assert(!connect_nothrow<decltype(ex::stopped_as_error(ex::just(1), 5))>);
static_assert(!connect_nothrow<decltype(ex::let_value(ex::just(1), [](int x) noexcept { return ex::just(x); }))>);
static_assert(!connect_nothrow<decltype(ex::let_error(ex::just_error(1), [](int x) noexcept { return ex::just(x); }))>);
static_assert(!connect_nothrow<decltype(ex::let_stopped(ex::just_stopped(), []() noexcept { return ex::just(1); }))>);
static_assert(!connect_nothrow<decltype(ex::continues_on(ex::just(1), ex::inline_scheduler{}))>);
static_assert(!connect_nothrow<decltype(ex::starts_on(ex::inline_scheduler{}, ex::just(1)))>);
static_assert(!connect_nothrow<decltype(ex::when_all(ex::just(1), ex::just(2)))>);
static_assert(!connect_nothrow<decltype(ex::bulk(ex::just(1), ex::seq, 2, [](int, int&) noexcept {}))>);

// a child whose connect may throw makes the adaptor's potentially throwing
struct ThrowingConnect {
  using sender_concept = ex::sender_tag;
  ex::env<> get_env() const noexcept { return {}; }
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(int)>{};
  }
  template <class Rcvr>
  struct Op {
    using operation_state_concept = ex::operation_state_tag;
    Rcvr rcvr;
    void start() & noexcept { ex::set_value(std::move(rcvr), 1); }
  };
  template <class Rcvr>
  Op<std::remove_cvref_t<Rcvr>> connect(Rcvr&& r) && {
    return {std::forward<Rcvr>(r)};
  }
};
static_assert(!connect_nothrow<ThrowingConnect>);
static_assert(!connect_nothrow<decltype(ex::then(ThrowingConnect{}, to_int))>);
static_assert(!connect_nothrow<decltype(ex::schedule_from(ThrowingConnect{}))>);
static_assert(!connect_nothrow<decltype(ex::write_env(ThrowingConnect{}, ex::env<>{}))>);
static_assert(!connect_nothrow<decltype(ex::into_variant(ThrowingConnect{}))>);


int main(int, char**) { return 0; }
