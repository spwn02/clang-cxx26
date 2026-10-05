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

// [exec.spawn.future]: a stop request on the consuming receiver's stop token cancels the future: after start()
// registered the receiver, the receiver completes with set_stopped as soon as the stop request arrives (try-cancel,
// try-set-stopped); the eagerly started input operation is asked to stop as well, and the state (with its scope
// association) lives until that operation has finished.

#include <atomic>
#include <cassert>
#include <execution>
#include <stop_token>
#include <thread>
#include <utility>

using namespace std::execution;

struct result {
  std::atomic<int> done{0};
  std::atomic<bool> stopped{false};
  std::atomic<int> value{0};
};

struct rcvr {
  using receiver_concept = receiver_tag;
  result* res;
  std::inplace_stop_token token;

  void set_value(int v) && noexcept {
    res->value = v;
    ++res->done;
  }
  void set_error(std::exception_ptr) && noexcept { assert(false); }
  void set_stopped() && noexcept {
    res->stopped = true;
    ++res->done;
  }
  auto get_env() const noexcept { return prop(std::get_stop_token, token); }
};

struct join_flag_rcvr {
  using receiver_concept = receiver_tag;
  std::atomic<bool>* flag;
  void set_value() && noexcept { *flag = true; }
  void set_stopped() && noexcept { assert(false); }
  auto get_env() const noexcept { return prop(get_start_scheduler, inline_scheduler{}); }
};

inline auto deferred(run_loop& loop, int v) { return schedule(loop.get_scheduler()) | then([v]() noexcept { return v; }); }

// A stop request after start completes the receiver with set_stopped right away, before the input operation (still
// queued on the loop) finishes; the association is released when it does.
void test_stop_after_start() {
  simple_counting_scope sc;
  run_loop loop;
  std::inplace_stop_source stop;
  result res;
  std::atomic<bool> joined{false};
  {
    auto fut = spawn_future(deferred(loop, 7), sc.get_token());
    auto op  = connect(std::move(fut), rcvr{&res, stop.get_token()});
    start(op);
    assert(res.done == 0);
    stop.request_stop();
    assert(res.done == 1 && res.stopped);

    auto jop = connect(sc.join(), join_flag_rcvr{&joined});
    start(jop);
    assert(!joined); // the input operation has not finished: the association is still held

    loop.finish();
    loop.run(); // the input operation was asked to stop and completes now
    assert(joined);
    assert(res.done == 1 && res.stopped); // completed exactly once
  }
}

// A stop request that precedes start() completes the receiver with set_stopped from start().
void test_stop_before_start() {
  simple_counting_scope sc;
  run_loop loop;
  std::inplace_stop_source stop;
  stop.request_stop();
  result res;
  std::atomic<bool> joined{false};
  {
    auto op = connect(spawn_future(deferred(loop, 7), sc.get_token()), rcvr{&res, stop.get_token()});
    start(op);
    assert(res.done == 1 && res.stopped);
    auto jop = connect(sc.join(), join_flag_rcvr{&joined});
    start(jop);
    assert(!joined);
    loop.finish();
    loop.run();
    assert(joined);
    assert(res.done == 1);
  }
}

// Without a stop request the result is delivered once; a stop request afterwards is ignored.
void test_value_then_late_stop() {
  simple_counting_scope sc;
  run_loop loop;
  std::inplace_stop_source stop;
  result res;
  {
    auto op = connect(spawn_future(deferred(loop, 7), sc.get_token()), rcvr{&res, stop.get_token()});
    start(op);
    loop.finish();
    loop.run();
    assert(res.done == 1 && !res.stopped && res.value == 7);
    stop.request_stop(); // the stop callback was deregistered when the receiver completed
    assert(res.done == 1);
  }
  std::this_thread::sync_wait(sc.join());
}

// A future whose input operation already completed delivers its value even if stop was requested before start().
void test_completed_future_ignores_stop() {
  simple_counting_scope sc;
  std::inplace_stop_source stop;
  stop.request_stop();
  result res;
  {
    auto op = connect(spawn_future(just(5), sc.get_token()), rcvr{&res, stop.get_token()});
    start(op);
    assert(res.done == 1 && !res.stopped && res.value == 5);
  }
  std::this_thread::sync_wait(sc.join());
}

// The input operation is asked to stop: a sender that reports stopped when stop was requested.
void test_input_sees_stop() {
  simple_counting_scope sc;
  run_loop loop;
  std::inplace_stop_source stop;
  result res;
  std::atomic<bool> input_saw_stop{false};
  {
    auto input = schedule(loop.get_scheduler()) | let_value([&]() noexcept {
                   return read_env(std::get_stop_token) | then([&](auto token) noexcept {
                            input_saw_stop = token.stop_requested();
                            return 1;
                          });
                 });
    auto op = connect(spawn_future(std::move(input), sc.get_token()), rcvr{&res, stop.get_token()});
    start(op);
    stop.request_stop();
    assert(res.done == 1 && res.stopped);
    loop.finish();
    loop.run();
    assert(input_saw_stop);
  }
  std::this_thread::sync_wait(sc.join());
}

// A stop request racing with the completion of the input operation completes the receiver exactly once, and the
// state is destroyed (the scope joins) in every interleaving.
void test_race() {
  for (int i = 0; i < 2000; ++i) {
    simple_counting_scope sc;
    run_loop loop;
    std::inplace_stop_source stop;
    result res;
    std::thread runner([&] { loop.run(); });
    {
      auto op = connect(spawn_future(deferred(loop, 7), sc.get_token()), rcvr{&res, stop.get_token()});
      start(op);
      stop.request_stop();
      loop.finish();
      runner.join();
      assert(res.done == 1);
    }
    std::this_thread::sync_wait(sc.join());
  }
}

// Abandoning a future that was never started while its input operation completes on another thread.
void test_abandon_race() {
  for (int i = 0; i < 2000; ++i) {
    simple_counting_scope sc;
    run_loop loop;
    std::thread runner([&] { loop.run(); });
    {
      auto fut = spawn_future(deferred(loop, 7), sc.get_token());
    } // abandoned
    loop.finish();
    runner.join();
    std::this_thread::sync_wait(sc.join());
  }
}

int main(int, char**) {
  test_stop_after_start();
  test_stop_before_start();
  test_value_then_late_stop();
  test_completed_future_ignores_stop();
  test_input_sees_stop();
  test_race();
  test_abandon_race();
  return 0;
}
