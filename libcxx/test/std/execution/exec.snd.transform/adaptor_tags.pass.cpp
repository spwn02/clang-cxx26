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

// The senders of bulk, when_all_with_variant, stopped_as_error, schedule_from and starts_on are make-sender(tag, data,
// children...) senders: they keep the tag of their algorithm until connect, when transform_sender lowers them to the
// senders they are expressed in ([exec.bulk], [exec.when.all], [exec.stopped.err], [exec.schedule.from],
// [exec.starts.on]). A sender of another tag is not transformed. (That a domain can intercept the lowering depends on
// the completion domain the attributes of these senders answer with, which is not implemented yet: #262.)

#include <cassert>
#include <concepts>
#include <exception>
#include <execution>
#include <thread>
#include <tuple>
#include <type_traits>
#include <variant>

namespace ex = std::execution;

struct CountingRcvr {
  using receiver_concept = ex::receiver_tag;
  int* value;
  int* errors;
  int* stopped;
  template <class... Ts>
  void set_value(Ts&&...) && noexcept {
    ++*value;
  }
  template <class E>
  void set_error(E&&) && noexcept {
    ++*errors;
  }
  void set_stopped() && noexcept { ++*stopped; }
  auto get_env() const noexcept { return ex::env<>{}; }
};

template <class Sndr, class Tag>
concept sender_for = ex::__sender_for<Sndr, Tag>;

template <class Tag, class Sndr>
concept can_transform = requires(Sndr&& s) { Tag::transform_sender(ex::set_value, std::forward<Sndr>(s), ex::env<>{}); };

template <class Sndr>
auto run(Sndr&& sndr, int& value, int& errors, int& stopped) {
  auto op = ex::connect(std::forward<Sndr>(sndr), CountingRcvr{&value, &errors, &stopped});
  ex::start(op);
}

void test_bulk() {
  auto sndr = ex::bulk(ex::just(1), ex::seq, 3, [](int i, int& x) noexcept { x += i; });
  static_assert(sender_for<decltype(sndr), ex::bulk_t>);
  static_assert(!sender_for<decltype(sndr), ex::bulk_chunked_t>);
  static_assert(can_transform<ex::bulk_t, decltype(sndr)&>);
  static_assert(!can_transform<ex::bulk_t, decltype(ex::just(1))>);
  // lowered to bulk_chunked
  auto lowered = ex::transform_sender(sndr, ex::env<>{});
  static_assert(sender_for<decltype(lowered), ex::bulk_chunked_t>);
  static_assert(std::same_as<decltype(ex::default_domain::transform_sender(ex::set_value, sndr, ex::env<>{})),
                             decltype(lowered)>);
  // the other forms keep their tag
  static_assert(sender_for<decltype(ex::bulk_chunked(ex::just(1), ex::seq, 3, [](int, int, int&) {})),
                           ex::bulk_chunked_t>);
  static_assert(sender_for<decltype(ex::bulk_unchunked(ex::just(1), ex::seq, 3, [](int, int&) {})),
                           ex::bulk_unchunked_t>);
  // and the sender does the work
  auto r = std::this_thread::sync_wait(ex::bulk(ex::just(1), ex::seq, 4, [](int i, int& x) noexcept { x += i; }));
  assert(r && std::get<0>(*r) == 1 + 0 + 1 + 2 + 3);
  // the closure forms
  auto r2 = std::this_thread::sync_wait(ex::just(1) | ex::bulk(ex::seq, 4, [](int i, int& x) noexcept { x += i; }));
  assert(r2 && std::get<0>(*r2) == 7);
}

void test_when_all_with_variant() {
  auto sndr = ex::when_all_with_variant(ex::just(1), ex::just(2.0));
  static_assert(sender_for<decltype(sndr), ex::when_all_with_variant_t>);
  static_assert(!sender_for<decltype(sndr), ex::when_all_t>);
  static_assert(can_transform<ex::when_all_with_variant_t, decltype(sndr)&>);
  static_assert(!can_transform<ex::when_all_with_variant_t, decltype(ex::just(1))>);
  using Lowered = decltype(ex::transform_sender(sndr, ex::env<>{}));
  static_assert(sender_for<Lowered, ex::when_all_t>);
  auto r = std::this_thread::sync_wait(ex::when_all_with_variant(ex::just(1), ex::just(2.0)));
  assert(r);
  assert(std::get<0>(std::get<std::tuple<int>>(std::get<0>(*r))) == 1);
  assert(std::get<0>(std::get<std::tuple<double>>(std::get<1>(*r))) == 2.0);
}

void test_stopped_as_error() {
  auto sndr = ex::stopped_as_error(ex::just_stopped(), 42);
  static_assert(sender_for<decltype(sndr), ex::stopped_as_error_t>);
  static_assert(can_transform<ex::stopped_as_error_t, decltype(sndr)&>);
  static_assert(!can_transform<ex::stopped_as_error_t, decltype(ex::just(1))>);
  using Lowered = decltype(ex::transform_sender(sndr, ex::env<>{}));
  static_assert(sender_for<Lowered, ex::let_stopped_t>);
  int value = 0, errors = 0, stopped = 0;
  run(ex::stopped_as_error(ex::just_stopped(), 42), value, errors, stopped);
  assert(value == 0 && errors == 1 && stopped == 0);
  // the closure form
  run(ex::just_stopped() | ex::stopped_as_error(42), value, errors, stopped);
  assert(errors == 2);
}

void test_schedule_from() {
  auto sndr = ex::schedule_from(ex::just(1));
  static_assert(sender_for<decltype(sndr), ex::schedule_from_t>);
  // no transformation of its own: the sender is its own, forwarding the completions of the child
  int value = 0, errors = 0, stopped = 0;
  run(ex::schedule_from(ex::just(1)), value, errors, stopped);
  assert(value == 1);
  auto r = std::this_thread::sync_wait(ex::schedule_from(ex::just(5)));
  assert(r && std::get<0>(*r) == 5);
}

void test_starts_on() {
  auto sndr = ex::starts_on(ex::inline_scheduler{}, ex::just(3));
  static_assert(sender_for<decltype(sndr), ex::starts_on_t>);
  static_assert(can_transform<ex::starts_on_t, decltype(sndr)&>);
  static_assert(!can_transform<ex::starts_on_t, decltype(ex::just(1))>);
  using Lowered = decltype(ex::transform_sender(sndr, ex::env<>{}));
  static_assert(sender_for<Lowered, ex::let_value_t>);
  auto r = std::this_thread::sync_wait(ex::starts_on(ex::inline_scheduler{}, ex::just(3)));
  assert(r && std::get<0>(*r) == 3);
}

int main(int, char**) {
  test_bulk();
  test_when_all_with_variant();
  test_stopped_as_error();
  test_schedule_from();
  test_starts_on();
  return 0;
}
