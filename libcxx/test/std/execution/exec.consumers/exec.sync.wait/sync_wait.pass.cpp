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

// namespace this_thread { inline constexpr unspecified sync_wait{}; }

#include <cassert>
#include <execution>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>

using namespace std::execution;

// Minimal hand-written senders, each offering both a value completion and one other
// completion kind, used below to exercise sync_wait's error/stopped paths. (just_error(...)
// and just_stopped() alone have no *value* completion at all, so neither can be sync_wait'd
// directly -- sync_wait's Mandates require `sync-wait-result-type<Sndr>`, which needs a
// value completion, to be well-formed -- see the comments at their use below.)
template <class Rcvr>
struct maybe_stops_opstate {
  using operation_state_concept = operation_state_tag;
  Rcvr rcvr;
  void start() & noexcept { set_stopped(std::move(rcvr)); }
};

struct maybe_stops_sndr {
  using sender_concept = sender_tag;

  template <class Rcvr>
  auto connect(Rcvr&& rcvr) && -> maybe_stops_opstate<std::remove_cvref_t<Rcvr>> {
    return {std::forward<Rcvr>(rcvr)};
  }

  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return completion_signatures<set_value_t(int), set_stopped_t()>{};
  }
};

template <class Rcvr>
struct maybe_errors_opstate {
  using operation_state_concept = operation_state_tag;
  Rcvr rcvr;
  void start() & noexcept { set_error(std::move(rcvr), std::runtime_error("boom")); }
};

struct maybe_errors_sndr {
  using sender_concept = sender_tag;

  template <class Rcvr>
  auto connect(Rcvr&& rcvr) && -> maybe_errors_opstate<std::remove_cvref_t<Rcvr>> {
    return {std::forward<Rcvr>(rcvr)};
  }

  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return completion_signatures<set_value_t(int), set_error_t(std::runtime_error)>{};
  }
};

int main(int, char**) {
  // Value completion: engaged optional holding a tuple of the results.
  {
    auto r = std::this_thread::sync_wait(just(42));
    assert(r.has_value());
    assert(std::get<0>(*r) == 42);
  }
  {
    auto r = std::this_thread::sync_wait(just(1, 'a'));
    assert(r.has_value());
    assert((*r == std::tuple<int, char>(1, 'a')));
  }
  {
    // just() -> a value completion with no datums: still an engaged optional<tuple<>>.
    auto r = std::this_thread::sync_wait(just());
    assert(r.has_value());
  }
  // Error completion: rethrows.
  {
    bool caught = false;
    try {
      std::this_thread::sync_wait(maybe_errors_sndr{});
    } catch (const std::runtime_error& e) {
      caught = true;
      assert(std::string(e.what()) == "boom");
    }
    assert(caught);
  }
  // Stopped completion: disengaged optional.
  {
    auto r = std::this_thread::sync_wait(maybe_stops_sndr{});
    assert(!r.has_value());
  }
  return 0;
}
