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
//   class counting_scope { ... };
//   struct associate_t { ... };
//   inline constexpr associate_t associate{};
//   struct spawn_t { ... };
//   inline constexpr spawn_t spawn{};
// }
//
// Pass 1 (simple_counting_scope/associate/spawn/join) and Pass 2 (counting_scope) are tested
// here (see docs/design/async_scope_p3149.md); spawn_future is explicit follow-up work, not
// tested here.

#include <cassert>
#include <execution>
#include <optional>
#include <stop_token>
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

// [exec.scope.counting] (Pass 2): counting_scope has the same association/close/join behavior
// as simple_counting_scope -- these mirror the Pass 1 tests above exactly, just through
// counting_scope, to confirm wrap()'s added stop-when composition doesn't disturb any of it.
static_assert(scope_token<counting_scope::token>);

void test_counting_scope_basic_token() {
  counting_scope scope;
  auto token = scope.get_token();
  assert(token.try_associate());
  token.disassociate();
}

void test_counting_scope_try_associate_after_close() {
  counting_scope scope;
  auto token = scope.get_token();
  scope.close();
  assert(!token.try_associate());
}

void test_counting_scope_spawn_and_join_deferred() {
  run_loop loop;
  counting_scope scope;
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

// request_stop() is independent of close()/join(): it only affects what stop token wrapped
// senders observe, not association bookkeeping.
void test_counting_scope_request_stop_independent_of_associate() {
  counting_scope scope;
  auto token = scope.get_token();
  scope.request_stop();
  assert(token.try_associate());
  token.disassociate();
}

// A receiver providing no stop token of its own (env<>{}, same as every other test above)
// answers get_stop_token with never_stop_token -- [exec.stop.when]p3.1: wrap()'s child sees the
// scope's own stop-source token directly, un-combined.
struct stop_token_capture_rcvr {
  using receiver_concept = receiver_tag;
  std::optional<std::inplace_stop_token>* captured;

  void set_value(std::inplace_stop_token tok) && noexcept { *captured = tok; }
  void set_error(std::exception_ptr) && noexcept { assert(false); }
  void set_stopped() && noexcept { assert(false); }
  auto get_env() const noexcept { return env<>{}; }
};

void test_counting_scope_wrap_unstoppable_branch() {
  counting_scope scope;
  auto token = scope.get_token();

  std::optional<std::inplace_stop_token> captured;
  auto op = connect(token.wrap(read_env(std::get_stop_token)), stop_token_capture_rcvr{&captured});
  start(op);
  assert(captured.has_value());
  assert(!captured->stop_requested());

  scope.request_stop();
  assert(captured->stop_requested());
}

// A receiver whose own environment answers get_stop_token with a genuinely stoppable token --
// [exec.stop.when]p3.2: wrap()'s child sees a combined token that reports stop_requested() as
// soon as EITHER side (the scope, or the receiver's own external source) requests it.
struct external_stop_token_rcvr {
  using receiver_concept = receiver_tag;
  std::optional<std::inplace_stop_token>* captured;
  std::inplace_stop_token external;

  void set_value(std::inplace_stop_token tok) && noexcept { *captured = tok; }
  void set_error(std::exception_ptr) && noexcept { assert(false); }
  void set_stopped() && noexcept { assert(false); }

  struct env_t {
    std::inplace_stop_token tok;
    auto query(std::get_stop_token_t) const noexcept { return tok; }
  };
  auto get_env() const noexcept { return env_t{external}; }
};

void test_counting_scope_wrap_combined_branch_scope_side() {
  counting_scope scope;
  auto token = scope.get_token();
  std::inplace_stop_source external_source;

  std::optional<std::inplace_stop_token> captured;
  auto op = connect(token.wrap(read_env(std::get_stop_token)),
                     external_stop_token_rcvr{&captured, external_source.get_token()});
  start(op);
  assert(captured.has_value());
  assert(!captured->stop_requested());

  scope.request_stop();
  assert(captured->stop_requested());
}

void test_counting_scope_wrap_combined_branch_external_side() {
  counting_scope scope;
  auto token = scope.get_token();
  std::inplace_stop_source external_source;

  std::optional<std::inplace_stop_token> captured;
  auto op = connect(token.wrap(read_env(std::get_stop_token)),
                     external_stop_token_rcvr{&captured, external_source.get_token()});
  start(op);
  assert(captured.has_value());
  assert(!captured->stop_requested());

  external_source.request_stop();
  assert(captured->stop_requested());
}

int main(int, char**) {
  test_basic_token();
  test_try_associate_after_close();
  test_associate_success();
  test_associate_closed();
  test_spawn_and_join_synchronous();
  test_spawn_and_join_deferred();

  test_counting_scope_basic_token();
  test_counting_scope_try_associate_after_close();
  test_counting_scope_spawn_and_join_deferred();
  test_counting_scope_request_stop_independent_of_associate();
  test_counting_scope_wrap_unstoppable_branch();
  test_counting_scope_wrap_combined_branch_scope_side();
  test_counting_scope_wrap_combined_branch_external_side();

  return 0;
}
