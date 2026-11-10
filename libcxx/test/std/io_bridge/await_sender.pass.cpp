//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// REQUIRES: std-at-least-c++29
// UNSUPPORTED: no-threads
// ADDITIONAL_COMPILE_FLAGS: -fexperimental-library

// <io_bridge>

// P4092R1: await_sender consumes a std::execution sender from an IoAwaitable coroutine.

#include <io_bridge>

#include <atomic>
#include <memory>
#include <string>
#include <tuple>
#include <type_traits>

#include "test_bridge.h"

using namespace test_bridge;
using test_io::test_context;

// A test_context runs everything on this thread.
template <class T>
std::optional<T> run_ctx(io::task<T> t) {
  test_context ctx;
  std::optional<T> out;
  io::run_async(ctx.get_executor(), [&](T&& v) { out.emplace(std::move(v)); }, [](std::exception_ptr) {})(std::move(t));
  ctx.run();
  return out;
}

io::task<int> just_value() { co_return co_await io::await_sender(ex::just(42)); }
io::task<void> just_void(bool* done) {
  co_await io::await_sender(ex::just());
  *done = true;
}
io::task<int> with_then() {
  co_return co_await io::await_sender(ex::just(6) | ex::then([](int v) { return v * 7; }));
}
io::task<std::string> with_let_value() {
  co_return co_await io::await_sender(
      ex::just(3) | ex::let_value([](int n) { return ex::just(std::string(std::size_t(n), 'x')); }));
}
io::task<std::tuple<int, std::string>> with_when_all() {
  auto [i, s] = co_await io::await_sender(ex::when_all(ex::just(5), ex::just(std::string("abc"))));
  co_return std::tuple<int, std::string>(i, s);
}
io::task<std::tuple<int, int>> multi_value() {
  co_return co_await io::await_sender(ex::just(1, 2));
}
io::task<int> move_only() {
  auto p = co_await io::await_sender(ex::just(std::make_unique<int>(9)));
  co_return *p;
}

// The sender runs on a run_loop thread; the coroutine resumes on its own io executor (a pool worker), not on the sender's
// thread.
io::task<void> across_threads(ex::run_loop* loop, std::thread::id* sender_thread, std::thread::id* before, std::thread::id* after) {
  *before = std::this_thread::get_id();
  int r   = co_await io::await_sender(ex::schedule(loop->get_scheduler()) | ex::then([sender_thread] {
                                        *sender_thread = std::this_thread::get_id();
                                        return 1764;
                                      }));
  assert(r == 1764);
  *after = std::this_thread::get_id();
}

io::task<io::io_result<int>> ec_value() { co_return co_await io::await_sender(ec_sender{5, {}}); }
io::task<io::io_result<int>> ec_failure() {
  co_return co_await io::await_sender(ec_sender{5, std::make_error_code(std::errc::io_error)});
}
io::task<io::io_result<>> ec_void_failure() {
  co_return co_await io::await_sender(ex::just_error(std::make_error_code(std::errc::connection_reset)));
}
io::task<io::io_result<int>> ec_stopped() { co_return co_await io::await_sender(ec_sender{0, {}, true}); }

#ifndef TEST_HAS_NO_EXCEPTIONS
io::task<int> stopped_throws() {
  try {
    co_await io::await_sender(ex::just_stopped());
  } catch (const std::system_error& e) {
    co_return e.code() == io::cond::canceled ? 1 : 2;
  }
  co_return 0;
}

io::task<int> thrown() {
  try {
    co_await io::await_sender(ex::just() | ex::then([] { TEST_THROW(7); }));
  } catch (int e) {
    co_return e;
  }
  co_return 0;
}
io::task<int> error_as_exception() {
  try {
    co_await io::await_sender(ex::just_error(7));
  } catch (int e) {
    co_return e;
  }
  co_return 0;
}
// A sender with both an error_code and an exception_ptr error: the exception is still rethrown.
io::task<int> ec_sender_throws() {
  try {
    auto [ec, v] = co_await io::await_sender(ec_sender{1, {}} | ex::then([](int) -> int { TEST_THROW(8); }));
    co_return ec ? -1 : v;
  } catch (int e) {
    co_return e;
  }
}

// The sender sees the stop token of the coroutine's io_env.
io::task<int> sees_stop_token(bool* stopped_seen) {
  co_await wait_for_stop{}; // wait until the test requests stop
  try {
    co_await io::await_sender(stop_waiting_sender{});
  } catch (const std::system_error& e) {
    *stopped_seen = e.code() == io::cond::canceled;
  }
  co_return 1;
}
#endif // TEST_HAS_NO_EXCEPTIONS

io::task<int> reads_token_in_sender(bool* possible) {
  *possible = co_await io::await_sender(ex::read_env(std::get_stop_token) | ex::then([](auto tok) { return tok.stop_possible(); }));
  co_return 0;
}

struct counting_op {
  static inline int constructed = 0;
};

int main(int, char**) {
  static_assert(io::IoAwaitable<decltype(io::await_sender(ex::just(1)))>);
  static_assert(std::is_same_v<decltype(std::declval<decltype(io::await_sender(ex::just(1)))&>().await_resume()), int>);
  static_assert(std::is_same_v<decltype(std::declval<decltype(io::await_sender(ex::just()))&>().await_resume()), void>);
  static_assert(std::is_same_v<decltype(std::declval<decltype(io::await_sender(ec_sender{}))&>().await_resume()),
                               io::io_result<int>>);
  static_assert(std::is_same_v<decltype(std::declval<decltype(io::await_sender(ex::just_error(std::error_code{})))&>().await_resume()),
                               io::io_result<>>);
  static_assert(std::is_same_v<decltype(std::declval<decltype(io::await_sender(ex::just(1, 2)))&>().await_resume()),
                               std::tuple<int, int>>);
  static_assert(!std::is_copy_constructible_v<decltype(io::await_sender(ex::just(1)))>);

  assert(*run_ctx(just_value()) == 42);
  {
    bool done = false;
    test_context ctx;
    io::run_async(ctx.get_executor())(just_void(&done));
    ctx.run();
    assert(done);
  }
  assert(*run_ctx(with_then()) == 42);
  assert(*run_ctx(with_let_value()) == "xxx");
  assert((*run_ctx(with_when_all()) == std::tuple<int, std::string>(5, "abc")));
  assert((*run_ctx(multi_value()) == std::tuple<int, int>(1, 2)));
  assert(*run_ctx(move_only()) == 9);

  // The completion is always posted to the coroutine's executor, even when the sender completes inline.
  {
    test_context ctx;
    int got = 0;
    io::run_async(ctx.get_executor(), [&](int v) { got = v; }, [](std::exception_ptr) {})(just_value());
    ctx.run();
    assert(got == 42);
    assert(ctx.posted >= 1);
  }

  // error_code signatures produce io_result.
  {
    auto [ec, v] = *run_ctx(ec_value());
    assert(!ec && v == 5);
  }
  {
    auto [ec, v] = *run_ctx(ec_failure());
    assert(ec == std::errc::io_error);
  }
  {
    auto [ec] = *run_ctx(ec_void_failure());
    assert(ec == std::errc::connection_reset);
  }
  {
    auto [ec, v] = *run_ctx(ec_stopped());
    assert(ec == io::cond::canceled); // stopped is error::canceled when the result is an io_result
  }

  // stopped: an exception without an error_code signature, error::canceled with one.
#ifndef TEST_HAS_NO_EXCEPTIONS
  assert(*run_ctx(stopped_throws()) == 1);
#endif

#ifndef TEST_HAS_NO_EXCEPTIONS
  assert(*run_ctx(thrown()) == 7);
  assert(*run_ctx(error_as_exception()) == 7);
  assert(*run_ctx(ec_sender_throws()) == 8);
#endif

  // Threads: the sender runs on the run_loop thread, the coroutine resumes on its own executor.
  {
    io::thread_pool pool(1);
    ex::run_loop loop;
    std::thread loop_thread([&] { loop.run(); });
    std::thread::id sender_thread, before, after;
    run_on_pool(pool, across_threads(&loop, &sender_thread, &before, &after));
    loop.finish();
    auto loop_id = loop_thread.get_id();
    loop_thread.join();
    assert(sender_thread == loop_id);
    assert(sender_thread != before);
    assert(before == after); // one worker: the coroutine resumed on its own executor
    assert(after != sender_thread);
    assert(before != std::this_thread::get_id());
    pool.join();
  }

#ifndef TEST_HAS_NO_EXCEPTIONS
  // Cancellation propagates through the io_env's stop token to the sender.
  {
    io::thread_pool pool(2);
    std::stop_source src;
    bool seen = false;
    std::promise<void> done;
    auto f   = done.get_future();
    auto exe = pool.get_executor();
    io::run_async(exe, src.get_token(), [&](int) { done.set_value(); }, [&](std::exception_ptr) { done.set_value(); })(sees_stop_token(&seen));
    src.request_stop();
    f.get();
    pool.join();
    assert(seen);
  }
#endif
  {
    bool possible = true;
    test_context ctx;
    io::run_async(ctx.get_executor())(reads_token_in_sender(&possible));
    ctx.run();
    assert(!possible); // run_async without a stop token: an empty std::stop_token
  }
  {
    std::stop_source src;
    bool possible = false;
    test_context ctx;
    io::run_async(ctx.get_executor(), src.get_token())(reads_token_in_sender(&possible));
    ctx.run();
    assert(possible);
  }

  return 0;
}
