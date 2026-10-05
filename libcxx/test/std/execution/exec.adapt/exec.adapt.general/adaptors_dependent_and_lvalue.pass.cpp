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

// [exec.adapt.general]: an adaptor whose child senders are all non-dependent is itself non-dependent: its completion
// signatures are known without an environment (computed for env<>{}, [exec.snd.expos]), and an adaptor of a dependent
// sender is dependent. [exec.snd.general]: a library sender has a connect that takes an rvalue sender, and one that takes
// an lvalue sender if it is copy constructible.

#include <cassert>
#include <concepts>
#include <execution>
#include <memory>
#include <thread>
#include <tuple>
#include <type_traits>

namespace ex = std::execution;

template <class S, class... Sigs>
constexpr bool has_sigs = std::same_as<ex::completion_signatures_of_t<S>, ex::completion_signatures<Sigs...>>;

template <class S>
concept non_dependent = ex::sender_in<S> && !ex::dependent_sender<S>;

// a user-defined dependent sender: its signatures can only be known with an environment
struct Dep {
  using sender_concept = ex::sender_tag;
  ex::env<> get_env() const noexcept { return {}; }
  template <class Self, class Env>
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
  Op<std::remove_cvref_t<Rcvr>> connect(Rcvr&& r) const noexcept {
    return {std::forward<Rcvr>(r)};
  }
};
static_assert(ex::dependent_sender<Dep>);

inline constexpr auto to_int = [](int x) noexcept { return x; };
inline constexpr auto to_void = [](int) noexcept {};
inline constexpr auto to_just = [](int x) noexcept { return ex::just(x); };
inline constexpr auto to_just_void = []() noexcept { return ex::just(); };

void test_non_dependent() {
  using V = ex::set_value_t;
  static_assert(non_dependent<decltype(ex::then(ex::just(1), to_int))>);
  static_assert(has_sigs<decltype(ex::then(ex::just(1), to_int)), V(int)>);
  static_assert(non_dependent<decltype(ex::upon_error(ex::just_error(1), to_int))>);
  static_assert(non_dependent<decltype(ex::upon_stopped(ex::just_stopped(), []() noexcept { return 1; }))>);
  static_assert(non_dependent<decltype(ex::let_value(ex::just(1), to_just))>);
  static_assert(has_sigs<decltype(ex::let_value(ex::just(1), to_just)), V(int)>);
  static_assert(non_dependent<decltype(ex::let_error(ex::just_error(1), to_just))>);
  static_assert(non_dependent<decltype(ex::let_stopped(ex::just_stopped(), to_just_void))>);
  static_assert(non_dependent<decltype(ex::continues_on(ex::just(1), ex::inline_scheduler{}))>);
  static_assert(non_dependent<decltype(ex::starts_on(ex::inline_scheduler{}, ex::just(1)))>);
  static_assert(non_dependent<decltype(ex::schedule_from(ex::just(1)))>);
  static_assert(non_dependent<decltype(ex::when_all(ex::just(1), ex::just(2)))>);
  static_assert(has_sigs<decltype(ex::when_all(ex::just(1), ex::just(2))), V(int, int)>);
  static_assert(non_dependent<decltype(ex::when_all_with_variant(ex::just(1)))>);
  static_assert(non_dependent<decltype(ex::into_variant(ex::just(1)))>);
  static_assert(non_dependent<decltype(ex::stopped_as_error(ex::just_stopped(), 1))>);
  static_assert(non_dependent<decltype(ex::stopped_as_optional(ex::just(1)))>);
  static_assert(non_dependent<decltype(ex::write_env(ex::just(1), ex::env<>{}))>);
  static_assert(non_dependent<decltype(ex::unstoppable(ex::just(1)))>);
  static_assert(non_dependent<decltype(ex::bulk(ex::just(1), ex::seq, 2, [](int, int&) noexcept {}))>);
  static_assert(non_dependent<decltype(ex::bulk_chunked(ex::just(1), ex::seq, 2, [](int, int, int&) noexcept {}))>);
  static_assert(non_dependent<decltype(ex::bulk_unchunked(ex::just(1), ex::seq, 2, [](int, int&) noexcept {}))>);
  // nested
  static_assert(non_dependent<decltype(ex::then(ex::let_value(ex::when_all(ex::just(1), ex::just(2)), [](int, int) noexcept {
                                         return ex::just();
                                       }),
                                       []() noexcept {}))>);
}

void test_child_asked_with_empty_environment() {
  // Without an environment the completion signatures of an adaptor are computed for env<>{} ([exec.snd.expos]): the
  // child is asked for its signatures with env<>{} as well, so a child that needs an environment but not a particular
  // one makes a non-dependent adaptor, and the adaptor has the signatures of the child for env<>{}.
  using V = ex::set_value_t;
  static_assert(non_dependent<decltype(ex::then(Dep{}, to_int))>);
  static_assert(has_sigs<decltype(ex::then(Dep{}, to_int)), V(int)>);
  static_assert(non_dependent<decltype(ex::let_value(Dep{}, to_just))>);
  static_assert(non_dependent<decltype(ex::continues_on(Dep{}, ex::inline_scheduler{}))>);
  static_assert(non_dependent<decltype(ex::when_all(Dep{}, ex::just(1)))>);
  static_assert(ex::sender_in<decltype(ex::then(Dep{}, to_int)), ex::env<>>);
  // Dep itself is the dependent sender: it has no signatures without an environment
  static_assert(ex::dependent_sender<Dep>);
}

struct IntRcvr {
  using receiver_concept = ex::receiver_tag;
  void set_value(int) && noexcept {}
};
template <class Sndr, class Rcvr>
concept can_connect = requires(Sndr&& s, Rcvr r) { ex::connect(std::forward<Sndr>(s), std::move(r)); };

void test_lvalue_connect() {
  // a copyable sender can be connected as an lvalue and used again afterwards
  auto sndr = ex::then(ex::just(1), [](int x) noexcept { return x + 1; });
  static_assert(std::copy_constructible<decltype(sndr)>);
  auto a = std::this_thread::sync_wait(sndr);
  auto b = std::this_thread::sync_wait(sndr);
  assert(a && b && std::get<0>(*a) == 2 && std::get<0>(*b) == 2);
  const auto csndr = ex::let_value(ex::just(2), to_just);
  auto c = std::this_thread::sync_wait(csndr);
  assert(c && std::get<0>(*c) == 2);
  auto when = ex::when_all(ex::just(1), ex::just(2));
  auto d = std::this_thread::sync_wait(when);
  auto e = std::this_thread::sync_wait(when);
  assert(d && e && std::get<1>(*d) == 2 && std::get<1>(*e) == 2);
  auto bulk = ex::bulk(ex::just(1), ex::seq, 3, [](int i, int& x) noexcept { x += i; });
  auto f = std::this_thread::sync_wait(bulk);
  auto g = std::this_thread::sync_wait(bulk);
  assert(f && g && std::get<0>(*f) == 4 && std::get<0>(*g) == 4);
  auto lvalue_just = ex::just(5);
  assert(std::get<0>(*std::this_thread::sync_wait(lvalue_just)) == 5);
  assert(std::get<0>(*std::this_thread::sync_wait(lvalue_just)) == 5);

  // a sender that cannot be copied has no lvalue connect
  auto move_only = ex::then(ex::just(std::make_unique<int>(1)), [](std::unique_ptr<int> p) noexcept { return *p; });
  static_assert(!std::copy_constructible<decltype(move_only)>);
  static_assert(can_connect<decltype(move_only), IntRcvr>);
  static_assert(!can_connect<decltype(move_only)&, IntRcvr>);
  static_assert(can_connect<decltype(sndr)&, IntRcvr>);
  static_assert(can_connect<const decltype(sndr)&, IntRcvr>);
}

int main(int, char**) {
  test_non_dependent();
  test_child_asked_with_empty_environment();
  test_lvalue_connect();
  return 0;
}
