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
//   template <class Token>
//   concept scope_token = ...;
//   class simple_counting_scope { ... };
//   struct associate_t { ... };
//   inline constexpr associate_t associate{};
//   struct spawn_t { ... };
//   inline constexpr spawn_t spawn{};
// }
//
// Pass 1 only (see docs/design/async_scope_p3149.md): counting_scope's stop-forwarding
// token::wrap() and spawn_future are explicit follow-up work, not tested here.

#include <cassert>
#include <execution>
#include <utility>

using namespace std::execution;

struct capture_rcvr {
  using receiver_concept = receiver_tag;
  bool* flag;

  void set_value() && noexcept { *flag = true; }
  void set_stopped() && noexcept { assert(false); }
  auto get_env() const noexcept { return env<>{}; }
};

struct stopped_rcvr {
  using receiver_concept = receiver_tag;
  bool* flag;

  void set_value() && noexcept { assert(false); }
  void set_stopped() && noexcept { *flag = true; }
  auto get_env() const noexcept { return env<>{}; }
};

static_assert(scope_token<simple_counting_scope::token>);

void test_basic_token() {
  simple_counting_scope scope;
  auto token = scope.get_token();
  assert(token.try_associate());
  token.disassociate();
}

// A closed scope rejects new associations.
void test_try_associate_after_close() {
  simple_counting_scope scope;
  auto token = scope.get_token();
  scope.close();
  assert(!token.try_associate());
}

void test_associate_success() {
  simple_counting_scope scope;
  auto token = scope.get_token();
  bool ran = false;
  auto op = connect(associate(just(), token), capture_rcvr{&ran});
  start(op);
  assert(ran);
}

// associate() on a closed scope completes with set_stopped(), never running the child sender.
void test_associate_closed() {
  simple_counting_scope scope;
  auto token = scope.get_token();
  scope.close();
  bool stopped = false;
  auto op = connect(associate(just(), token), stopped_rcvr{&stopped});
  start(op);
  assert(stopped);
}

// join() takes its synchronous fast path when nothing is outstanding.
void test_spawn_and_join_synchronous() {
  simple_counting_scope scope;
  auto token = scope.get_token();
  spawn(just(), token);
  bool joined = false;
  auto op = connect(scope.join(), capture_rcvr{&joined});
  start(op);
  assert(joined);
}

// The genuinely-deferred completion path: nothing in this fork's senders completes
// asynchronously by default, so run_loop's queue-then-run() split is what actually exercises
// "join() is still outstanding, then completes once the count reaches zero" rather than
// always taking the synchronous fast path (see this facility's design note).
void test_spawn_and_join_deferred() {
  run_loop loop;
  simple_counting_scope scope;
  auto token = scope.get_token();
  spawn(schedule(loop.get_scheduler()), token);

  bool joined = false;
  auto join_op = connect(scope.join(), capture_rcvr{&joined});
  start(join_op);
  assert(!joined);

  loop.finish();
  loop.run();
  assert(joined);
}

int main(int, char**) {
  test_basic_token();
  test_try_associate_after_close();
  test_associate_success();
  test_associate_closed();
  test_spawn_and_join_synchronous();
  test_spawn_and_join_deferred();

  return 0;
}
