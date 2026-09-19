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
//   struct spawn_future_t { ... };
//   inline constexpr spawn_future_t spawn_future{};
// }
//
// Pass 1 (simple_counting_scope/associate/spawn/join), Pass 2 (counting_scope), and Pass 3
// (spawn_future) are all tested here (see docs/design/async_scope_p3149.md).

#include <cassert>
#include <execution>
#include <optional>
#include <stdexcept>
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

// [exec.spawn.future] (Pass 3): a receiver capturing whichever of set_value/set_error/set_stopped
// the returned future sender completes with. Templated on the expected value type so the same
// helper covers every value-completing test below.
template <class T>
struct capture_value_rcvr {
  using receiver_concept = receiver_tag;
  std::optional<T>* value;
  bool* stopped;
  std::exception_ptr* error;

  void set_value(T v) && noexcept { *value = std::move(v); }
  void set_error(std::exception_ptr e) && noexcept { *error = e; }
  void set_stopped() && noexcept { *stopped = true; }
  auto get_env() const noexcept { return env<>{}; }
};

// Ordinary case: everything (association, the child sender, and consuming the result) happens
// synchronously -- complete() runs before consume() ever gets a chance to register, so consume()
// takes its "already complete" fast path (dispatching the stored result immediately).
void test_spawn_future_value_synchronous() {
  counting_scope scope;
  auto token = scope.get_token();

  std::optional<int> value;
  bool stopped = false;
  std::exception_ptr error;
  auto fut = spawn_future(just(42), token);
  auto op  = connect(std::move(fut), capture_value_rcvr<int>{&value, &stopped, &error});
  start(op);
  assert(value == 42);
  assert(!stopped);
  assert(!error);
}

// A closed scope: try_associate() fails in the state's own constructor, so the child sender is
// never started at all and the future completes with set_stopped() -- independent of the child.
void test_spawn_future_closed_scope_is_stopped() {
  counting_scope scope;
  auto token = scope.get_token();
  scope.close();

  std::optional<int> value;
  bool stopped = false;
  std::exception_ptr error;
  auto fut = spawn_future(just(42), token);
  auto op  = connect(std::move(fut), capture_value_rcvr<int>{&value, &stopped, &error});
  start(op);
  assert(!value.has_value());
  assert(stopped);
  assert(!error);
}

struct spawn_future_test_error : std::runtime_error {
  spawn_future_test_error() : std::runtime_error("spawn_future_test_error") {}
};

struct throwing_sndr {
  using sender_concept = sender_tag;

  template <class Rcvr>
  struct opstate {
    using operation_state_concept = operation_state_tag;
    Rcvr rcvr;
    void start() & noexcept { set_error(std::move(rcvr), std::make_exception_ptr(spawn_future_test_error{})); }
  };

  template <class Rcvr>
  auto connect(Rcvr&& rcvr) const {
    return opstate<std::decay_t<Rcvr>>{std::forward<Rcvr>(rcvr)};
  }
  auto get_env() const noexcept { return env<>{}; }

  template <class Self, class Env>
  static consteval auto get_completion_signatures() {
    return completion_signatures<set_error_t(std::exception_ptr)>{};
  }
};

void test_spawn_future_error() {
  counting_scope scope;
  auto token = scope.get_token();

  std::optional<int> value;
  bool stopped = false;
  std::exception_ptr error;
  auto fut = spawn_future(throwing_sndr{}, token);
  auto op  = connect(std::move(fut), capture_value_rcvr<int>{&value, &stopped, &error});
  start(op);
  assert(!value.has_value());
  assert(!stopped);
  assert(error);
  try {
    std::rethrow_exception(error);
    assert(false);
  } catch (const spawn_future_test_error&) {
  }
}

// The genuine abandonment case: the returned future sender is destroyed before ever being
// connected/started, while the underlying (scheduled-but-not-yet-run) operation is still
// outstanding -- this must request stop on it (__abandon()'s __phase::__initial branch), not
// just silently detach it. run_loop's queue-then-run() split is what makes the operation
// genuinely still outstanding at the point of abandonment, rather than already complete (see
// this facility's own design note on why every test here needs to be built around run_loop from
// the start, not retrofitted).
//
// The observable proof isn't "some downstream continuation saw stop_requested()": run_loop's own
// schedule() opstate checks its own stop token *before* dispatching
// ([exec.run.loop.types]p10.2, <__execution/run_loop.h>) and completes with set_stopped()
// directly the moment it sees one, without ever reaching anything chained after it -- so nothing
// downstream of schedule(loop.get_scheduler()) ever runs here at all. What this test checks
// instead is the property that actually matters: the scope's own association count returns to
// zero cleanly. That only happens if __abandon()'s request_stop() actually reached the
// still-outstanding operation, letting it complete (via set_stopped(), through
// __spawn_future_receiver), which drives __complete() to (eventually, once request_stop() has
// fully returned -- see the __request_stop_in_progress_ handling in __complete()/__abandon())
// call __destroy(), which disassociates the token. Without that, the association would leak and
// this scope's own destructor would std::terminate() -- exactly the bug an earlier version of
// this fix had, caught by this very test crashing instead of completing.
void test_spawn_future_abandon_requests_stop() {
  run_loop loop;
  counting_scope scope;
  auto token = scope.get_token();

  {
    auto fut = spawn_future(schedule(loop.get_scheduler()), token);
    // fut destroyed here, without ever being connected -- this is the abandonment.
  }

  loop.finish();
  loop.run();

  scope.close();
  auto r = std::this_thread::sync_wait(scope.join());
  assert(r.has_value());
}

// Abandoning a future whose underlying operation already completed (synchronously, as
// everything in this fork's test senders does unless explicitly scheduled via run_loop) just
// tears the state down cleanly -- __abandon()'s __phase::__completed branch, exercised by every
// test above that never calls connect() at all... except they all DO call connect(), so exercise
// it explicitly here by discarding the returned sender outright.
void test_spawn_future_abandon_after_synchronous_completion() {
  counting_scope scope;
  auto token = scope.get_token();
  spawn_future(just(1), token); // discarded immediately; already completed by the time it is
}

// consume()-registers-then-complete()-fires: the mirror image of the synchronous case above,
// and the other half of the race __complete()/__consume() must resolve correctly. Connecting and
// starting the future sender happens *before* the underlying scheduled work has run, so
// __consume() takes its "register and wait" path; running the loop afterward is what actually
// triggers __complete(), which must notice the registered receiver and dispatch to it.
void test_spawn_future_consume_then_complete_deferred() {
  run_loop loop;
  counting_scope scope;
  auto token = scope.get_token();

  std::optional<int> value;
  bool stopped = false;
  std::exception_ptr error;
  auto fut = spawn_future(schedule(loop.get_scheduler()) | let_value([] { return just(7); }), token);
  auto op  = connect(std::move(fut), capture_value_rcvr<int>{&value, &stopped, &error});
  start(op);
  assert(!value.has_value());
  assert(!stopped);

  loop.finish();
  loop.run();
  assert(value == 7);
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

  test_spawn_future_value_synchronous();
  test_spawn_future_closed_scope_is_stopped();
  test_spawn_future_error();
  test_spawn_future_abandon_requests_stop();
  test_spawn_future_abandon_after_synchronous_completion();
  test_spawn_future_consume_then_complete_deferred();

  return 0;
}
