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
#include <utility>
#include <type_traits>

namespace ex  = std::execution;
using assoc_t = decltype(std::declval<ex::simple_counting_scope::token>().try_associate());
// [exec.scope.concepts]: "concept scope_association = movable<Assoc> &&
// is_nothrow_move_constructible_v<Assoc> && is_nothrow_move_assignable_v<Assoc> &&
// default_initializable<Assoc> && requires(const Assoc assoc) {
//   { static_cast<bool>(assoc) } noexcept;
//   { assoc.try_associate() } -> same_as<Assoc>; };"
static_assert(ex::scope_association<assoc_t>);
static_assert(ex::scope_association<decltype(std::declval<ex::counting_scope::token>().try_associate())>);
static_assert(!std::same_as<assoc_t, bool>);
struct missing_wrap {
  assoc_t try_associate() const { return {}; }
};
// [exec.scope.concepts]: "{ token.wrap(declval<test-sender>()) } -> sender_in<test-env>;"
static_assert(!ex::scope_token<missing_wrap>);
static_assert(ex::scope_token<ex::simple_counting_scope::token>);
static_assert(ex::scope_token<ex::counting_scope::token>);
// [exec.scope.simple.counting.general], [exec.scope.counting]:
// "static constexpr size_t max_associations = unspecified;"
static_assert(std::same_as<decltype(ex::simple_counting_scope::max_associations), const std::size_t>);
static_assert(std::same_as<decltype(ex::counting_scope::max_associations), const std::size_t>);
static_assert(ex::simple_counting_scope::max_associations > 0);
static_assert(ex::counting_scope::max_associations > 0);

struct receiver {
  using receiver_concept = ex::receiver_tag;
  bool* done;
  decltype(std::declval<ex::run_loop&>().get_scheduler()) scheduler;
  void set_value() && noexcept {
    assert(!*done);
    *done = true;
  }
  void set_stopped() && noexcept { assert(false); }
  void set_error(std::exception_ptr) && noexcept { assert(false); }
  auto get_env() const noexcept { return ex::prop(ex::get_start_scheduler, scheduler); }
};

template <class Scope>
void test() {
  Scope scope;
  ex::run_loop loop;
  auto token = scope.get_token();
  using A    = decltype(token.try_associate());
  A empty;
  assert(!empty && !empty.try_associate());
  auto first  = token.try_associate();
  auto second = first.try_associate();
  assert(first && second);
  // [exec.scope.concepts]: "after it is used as the source operand of a move
  // constructor, the assoc is not engaged;" and move assignment releases the old association.
  A moved(std::move(first));
  assert(!first && moved);
  second = std::move(moved);
  assert(!moved && second);
  bool done1 = false, done2 = false;
  auto op1 = ex::connect(scope.join(), receiver{&done1, loop.get_scheduler()});
  auto op2 = ex::connect(scope.join(), receiver{&done2, loop.get_scheduler()});
  ex::start(op1);
  ex::start(op2);
  assert(!done1 && !done2);
  // [exec.simple.counting.mem]: "calls complete() on all objects registered with *this."
  // [exec.counting.scopes.general]: "void complete() noexcept { start(op); }"
  second = {};
  assert(!done1 && !done2); // both completions must await the receiver start scheduler
  loop.finish();
  loop.run();
  assert(done1 && done2);
  // [exec.simple.counting.mem]: try-associate increments in unused, open or
  // open-and-joining; "otherwise, no effects" and returns "assoc-t() otherwise."
  assert(!token.try_associate());
  assert(!first.try_associate());
}
int main(int, char**) {
  test<ex::simple_counting_scope>();
  test<ex::counting_scope>();
}
