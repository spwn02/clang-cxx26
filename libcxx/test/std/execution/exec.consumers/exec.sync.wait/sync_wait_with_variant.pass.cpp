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

// namespace this_thread { inline constexpr unspecified sync_wait_with_variant{}; }

// [exec.sync.wait.var]p3: sync_wait_with_variant(sndr) is equivalent to
// sync_wait(into_variant(sndr)).

#include <cassert>
#include <execution>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

using namespace std::execution;

// A sender declaring two distinct value-completion shapes -- exactly the case
// sync_wait (which requires a single decayed-tuple value type) can't handle directly,
// and into_variant/sync_wait_with_variant exists for.
template <class Rcvr>
struct two_shapes_opstate {
  using operation_state_concept = operation_state_tag;
  Rcvr rcvr;
  bool send_int;
  void start() & noexcept {
    if (send_int)
      set_value(std::move(rcvr), 42);
    else
      set_value(std::move(rcvr), std::string("hello"));
  }
};

struct two_shapes_sndr {
  using sender_concept = sender_tag;
  bool send_int;

  template <class Rcvr>
  auto connect(Rcvr&& rcvr) && -> two_shapes_opstate<std::remove_cvref_t<Rcvr>> {
    return {std::forward<Rcvr>(rcvr), send_int};
  }

  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return completion_signatures<set_value_t(int), set_value_t(std::string)>{};
  }
};

template <class Rcvr>
struct stops_sndr_opstate {
  using operation_state_concept = operation_state_tag;
  Rcvr rcvr;
  void start() & noexcept { set_stopped(std::move(rcvr)); }
};

struct stops_sndr {
  using sender_concept = sender_tag;

  template <class Rcvr>
  auto connect(Rcvr&& rcvr) && -> stops_sndr_opstate<std::remove_cvref_t<Rcvr>> {
    return {std::forward<Rcvr>(rcvr)};
  }

  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return completion_signatures<set_value_t(int), set_value_t(std::string), set_stopped_t()>{};
  }
};

int main(int, char**) {
  // sync_wait_with_variant(sndr) is sync_wait(into_variant(sndr)), and sync_wait's own
  // result type always wraps its completion args in a tuple -- so the result here is
  // optional<tuple<variant<...>>>, not optional<variant<...>> directly. into_variant's
  // single value-completion argument (the variant) becomes that lone tuple element.

  // Value completion, first alternative.
  {
    auto r = std::this_thread::sync_wait_with_variant(two_shapes_sndr{/*send_int=*/true});
    assert(r.has_value());
    auto& v = std::get<0>(*r);
    using V = std::decay_t<decltype(v)>;
    static_assert(std::variant_size_v<V> == 2);
    assert(std::holds_alternative<std::tuple<int>>(v));
    assert(std::get<std::tuple<int>>(v) == std::tuple<int>(42));
  }
  // Value completion, second alternative.
  {
    auto r = std::this_thread::sync_wait_with_variant(two_shapes_sndr{/*send_int=*/false});
    assert(r.has_value());
    auto& v = std::get<0>(*r);
    assert(std::holds_alternative<std::tuple<std::string>>(v));
    assert(std::get<std::tuple<std::string>>(v) == std::tuple<std::string>("hello"));
  }
  // Stopped completion: disengaged optional, matching sync_wait's own behavior.
  {
    auto r = std::this_thread::sync_wait_with_variant(stops_sndr{});
    assert(!r.has_value());
  }
  // Equivalent to sync_wait(into_variant(sndr)) -- spot-check the two spellings agree.
  {
    auto r1 = std::this_thread::sync_wait_with_variant(two_shapes_sndr{/*send_int=*/true});
    auto r2 = std::this_thread::sync_wait(into_variant(two_shapes_sndr{/*send_int=*/true}));
    static_assert(std::is_same_v<decltype(r1), decltype(r2)>);
    assert(r1 == r2);
  }
  return 0;
}
