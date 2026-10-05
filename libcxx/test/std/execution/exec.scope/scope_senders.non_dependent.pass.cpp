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

// [exec.associate], [exec.spawn.future], [exec.scope.simple.counting], [exec.scope.counting]: these senders are
// basic-senders with an environment-independent completion (given an environment-independent child): they are not
// dependent senders and are sender_in without an environment.

#include <cassert>
#include <exception>
#include <execution>
#include <thread>
#include <type_traits>

namespace ex = std::execution;

template <class Sndr, class Sigs>
constexpr bool non_dependent_with =
    !ex::dependent_sender<Sndr> && ex::sender_in<Sndr> && ex::sender_in<Sndr, ex::env<>> &&
    std::is_same_v<ex::completion_signatures_of_t<Sndr, ex::env<>>, Sigs>;

int main(int, char**) {
  ex::simple_counting_scope sc;
  ex::counting_scope cs;
  {
    auto tok = sc.get_token();
    using assoc_sigs = ex::completion_signatures<ex::set_value_t(int), ex::set_stopped_t()>;
    auto a           = ex::associate(ex::just(1), tok);
    static_assert(non_dependent_with<decltype(a), assoc_sigs>);
    auto b = ex::just(1) | ex::associate(tok);
    static_assert(non_dependent_with<decltype(b), assoc_sigs>);
    auto c = ex::associate(ex::just(1), cs.get_token());
    static_assert(non_dependent_with<decltype(c), assoc_sigs>);

    // an associate over a child that is dependent is dependent
    auto d = ex::associate(ex::read_env(ex::get_start_scheduler), tok);
    static_assert(ex::dependent_sender<decltype(d)>);

    auto f = ex::spawn_future(ex::just(1), tok);
    static_assert(!ex::dependent_sender<decltype(f)>);
    static_assert(ex::sender_in<decltype(f)>);
    static_assert(ex::sender_in<decltype(f), ex::env<>>);
    auto r = std::this_thread::sync_wait(std::move(f));
    assert(r && std::get<0>(*r) == 1);
  }
  static_assert(non_dependent_with<decltype(sc.join()), ex::completion_signatures<ex::set_value_t()>>);
  static_assert(non_dependent_with<decltype(cs.join()), ex::completion_signatures<ex::set_value_t()>>);
  std::this_thread::sync_wait(sc.join());
  std::this_thread::sync_wait(cs.join());
  return 0;
}
