//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <execution>

// [exec.connect]: connect(sndr, rcvr) is expression-equivalent to new_sndr.connect(rcvr) where
// new_sndr is transform_sender(sndr, get_env(rcvr)), so it is potentially throwing exactly if the transformation or
// the member connect is.

#include <cassert>
#include <concepts>
#include <execution>
#include <type_traits>
#include <utility>

namespace ex = std::execution;

struct Op {
  using operation_state_concept = ex::operation_state_tag;
  void start() & noexcept {}
};

struct Rcvr {
  using receiver_concept = ex::receiver_tag;
  void set_value() && noexcept {}
};

template <bool Nothrow>
struct MemberSender {
  using sender_concept = ex::sender_tag;
  ex::env<> get_env() const noexcept { return {}; }
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t()>{};
  }
  Op connect(Rcvr) const noexcept(Nothrow) { return {}; }
};

static_assert(noexcept(std::declval<const MemberSender<true>&>().connect(Rcvr{})));
static_assert(noexcept(ex::connect(std::declval<const MemberSender<true>&>(), Rcvr{})));
static_assert(!noexcept(std::declval<const MemberSender<false>&>().connect(Rcvr{})));
static_assert(!noexcept(ex::connect(std::declval<const MemberSender<false>&>(), Rcvr{})));

// a receiver whose environment makes the transformation of the sender potentially throwing
struct ThrowingDomain {
  template <class Tag, class Sndr, class Env>
  static constexpr auto transform_sender(Tag, Sndr&& sndr, const Env&) {
    return std::forward<Sndr>(sndr);
  }
};
struct ThrowingEnv {
  ThrowingDomain query(ex::get_domain_t) const noexcept { return {}; }
};
struct ThrowingEnvRcvr {
  using receiver_concept = ex::receiver_tag;
  void set_value() && noexcept {}
  ThrowingEnv get_env() const noexcept { return {}; }
};
template <bool Nothrow>
struct EnvSender {
  using sender_concept = ex::sender_tag;
  ex::env<> get_env() const noexcept { return {}; }
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t()>{};
  }
  Op connect(ThrowingEnvRcvr) const noexcept(Nothrow) { return {}; }
};
static_assert(!noexcept(ex::connect(std::declval<const EnvSender<true>&>(), ThrowingEnvRcvr{})));
static_assert(!noexcept(ex::connect(std::declval<const EnvSender<false>&>(), ThrowingEnvRcvr{})));

int main(int, char**) {
  MemberSender<true> s;
  auto op = ex::connect(s, Rcvr{});
  ex::start(op);
  return 0;
}
