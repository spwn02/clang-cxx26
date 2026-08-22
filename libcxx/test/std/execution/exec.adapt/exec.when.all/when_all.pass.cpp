//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: no-threads

// <execution>

// namespace execution {
//   struct when_all_t { ... };
//   inline constexpr when_all_t when_all{};
// }

#include <cassert>
#include <execution>
#include <type_traits>
#include <utility>

using namespace std::execution;

// A child sender that unconditionally completes with set_error(99) as soon as it's started --
// used (alongside stop_check_sndr below) to exercise when_all's stop-propagation-on-error path
// without needing real concurrency: when_all starts every child in argument order via a fold
// expression, so a synchronously-completing first child has already requested stop (and
// updated disp) before the second child's start() runs.
struct error_sndr {
  using sender_concept = sender_tag;

  template <class _Rcvr>
  struct __opstate {
    using operation_state_concept = operation_state_tag;
    _Rcvr rcvr;
    void start() & noexcept { std::execution::set_error(std::move(rcvr), 99); }
  };

  template <class _Rcvr>
  auto connect(_Rcvr&& __rcvr) && -> __opstate<std::remove_cvref_t<_Rcvr>> {
    return {std::forward<_Rcvr>(__rcvr)};
  }

  template <class _Self, class... _Env>
  static consteval auto get_completion_signatures() {
    return completion_signatures<set_error_t(int)>{};
  }
};
static_assert(sender<error_sndr>);

// A child sender that records, via a pointer to a caller-owned bool, whether its own
// get_stop_token(get_env(rcvr)) already reports stop_requested() at the moment it starts --
// the direct observation that when-all-env's get_stop_token interception (and
// __when_all_on_stop_request's propagation from a sibling's error) actually reached this
// child, since when_all's own aggregate completion (whichever child "wins") can't otherwise
// expose what a losing child individually observed.
struct stop_check_sndr {
  using sender_concept = sender_tag;
  bool* observed;

  template <class _Rcvr>
  struct __opstate {
    using operation_state_concept = operation_state_tag;
    bool* observed;
    _Rcvr rcvr;
    void start() & noexcept {
      *observed = std::get_stop_token(std::execution::get_env(rcvr)).stop_requested();
      std::execution::set_value(std::move(rcvr));
    }
  };

  template <class _Rcvr>
  auto connect(_Rcvr&& __rcvr) && -> __opstate<std::remove_cvref_t<_Rcvr>> {
    return {observed, std::forward<_Rcvr>(__rcvr)};
  }

  template <class _Self, class... _Env>
  static consteval auto get_completion_signatures() {
    return completion_signatures<set_value_t()>{};
  }
};
static_assert(sender<stop_check_sndr>);

int main(int, char**) {
  // Value concatenation: each child's own value datums, concatenated in argument order.
  {
    auto r = std::this_thread::sync_wait(when_all(just(1, 2), just(3.5)));
    assert(r.has_value());
    assert(*r == std::tuple(1, 2, 3.5));
    static_assert(std::is_same_v<completion_signatures_of_t<decltype(when_all(just(1, 2), just(3.5))), env<>>,
                                  completion_signatures<set_value_t(int, int, double)>>);
  }
  // [exec.when.all]p13's collapse case: a child with zero set_value shapes (just_stopped) makes
  // the whole values_tuple tuple<>, so the aggregate's own value completion is the datum-less
  // set_value_t() -- unreachable here since just_stopped actually fires, making the aggregate
  // complete stopped instead (no value at all).
  {
    auto r = std::this_thread::sync_wait(when_all(just_stopped(), just(1)));
    assert(!r.has_value());
    static_assert(std::is_same_v<completion_signatures_of_t<decltype(when_all(just_stopped(), just(1))), env<>>,
                                  completion_signatures<set_value_t(), set_stopped_t()>>);
  }
  // No error/stopped-capable child anywhere: when_all(just(...), just(...)) advertises no
  // set_error_t/set_stopped_t completion at all, not even set_error_t(exception_ptr) (every
  // datum here is trivially nothrow-decay-copyable, so copy-fail is none-such).
  {
    static_assert(!sends_stopped<decltype(when_all(just(1), just(2))), env<>>);
    static_assert(std::is_same_v<completion_signatures_of_t<decltype(when_all(just(1), just(2))), env<>>,
                                  completion_signatures<set_value_t(int, int)>>);
  }
  // Error propagation + stop-request reaching a sibling child: error_sndr completes first
  // (synchronously, during when_all's own start()), requesting stop on this operation's
  // inplace_stop_source before stop_check_sndr's own start() runs -- confirming when-all-env's
  // get_stop_token interception actually threads the shared stop source through to every
  // child's receiver environment, not just the one that errored.
  {
    bool observed = false;
    bool caught   = false;
    try {
      (void)std::this_thread::sync_wait(when_all(error_sndr{}, stop_check_sndr{&observed}));
    } catch (int __err) {
      caught = true;
      assert(__err == 99);
    }
    assert(caught);
    assert(observed);
  }
  return 0;
}
