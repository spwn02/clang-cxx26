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
#include <execution>
#include <cassert>
namespace ex = std::execution;
struct query_tag {
  static constexpr bool query(std::forwarding_query_t) noexcept { return true; }
  template <class E>
  int operator()(const E& e) const noexcept {
    if constexpr (requires { e.query(*this); })
      return e.query(*this);
    else
      return -1;
  }
};
int main(int, char**) {
  {
    ex::simple_counting_scope scope;
    // [exec.associate]: "explicit associate-data(Token t, Sender&& s)
    // : sndr(t.wrap(std::forward<Sender>(s))), assoc([&] {
    //   sender-ref guard{addressof(sndr)}; auto assoc = t.try_associate();
    //   if (assoc) { guard.release(); } return assoc; }()) {}"
    auto input  = ex::just(7);
    auto sender = ex::associate(std::move(input), scope.get_token());
    auto copy   = sender;
    scope.close();
    {
      auto result = std::this_thread::sync_wait(std::move(sender));
      assert(result && std::get<0>(*result) == 7);
      auto other = std::this_thread::sync_wait(std::move(copy));
      assert(other && std::get<0>(*other) == 7);
    }
    std::this_thread::sync_wait(scope.join());
  }
  {
    ex::simple_counting_scope scope;
    int value = 0;
    // [exec.spawn]: "Uses alloc to allocate and construct an object o of type
    // decltype(spawn-state(alloc, write_env(token.wrap(sndr), senv), token))
    // from alloc, write_env(token.wrap(sndr), senv), and token and then invokes o.run()."
    ex::spawn(ex::read_env(query_tag{}) | ex::then([&](int x) noexcept { value = x; }),
              scope.get_token(),
              ex::prop(query_tag{}, 42));
    assert(value == 42);
    std::this_thread::sync_wait(scope.join());
  }
}
