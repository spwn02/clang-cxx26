//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23, c++26
// UNSUPPORTED: no-threads
// ADDITIONAL_COMPILE_FLAGS: -fexperimental-library
// <io>

// std::io::when_all: concurrent children of the awaiting task, tuple of values, void children, exception propagation
// with cancellation of the siblings, the range form, environment propagation, and use on a thread_pool.

#include <io>

#include <atomic>
#include <cassert>
#include <stdexcept>
#include <stop_token>
#include <string>
#include <tuple>
#include <vector>

#include "test_io.h"
#include "test_macros.h"

namespace io = std::io;
using namespace test_io;

struct observed {
  io::execution_context* ctx;
  bool stop_possible;
  bool stop_requested;
  std::pmr::memory_resource* mr;
};

io::task<observed> observe() {
  const io::io_env* env = co_await io::get_environment;
  co_return observed{
      &env->executor.context(),
      env->stop_token.stop_possible(),
      env->stop_token.stop_requested(),
      env->frame_allocator};
}

io::task<int> value(int v, int hops = 0) {
  for (int i = 0; i != hops; ++i)
    co_await post_op{};
  co_return v;
}

io::task<std::string> text(std::string s) { co_return s; }

io::task<void> note(int& slot, int v, int hops = 0) {
  for (int i = 0; i != hops; ++i)
    co_await post_op{};
  slot = v;
}

// Keeps hopping until stop is requested; counts itself finished.
io::task<void> until_stopped(int& finished, int& observed_stop) {
  const io::io_env* env = co_await io::get_environment;
  for (int i = 0; i != 100000 && !env->stop_token.stop_requested(); ++i)
    co_await post_op{};
  if (env->stop_token.stop_requested())
    ++observed_stop;
  ++finished;
}

#ifndef TEST_HAS_NO_EXCEPTIONS
io::task<int> boom(int hops) {
  for (int i = 0; i != hops; ++i)
    co_await post_op{};
  throw std::runtime_error("boom");
  co_return 0;
}
io::task<void> boom_void() {
  co_await post_op{};
  throw 17;
}
#endif

// Tasks that use when_all.
io::task<void> values_test(int& a, std::string& b) {
  auto [x, y, z] = co_await io::when_all(value(1, 3), text("two"), value(3));
  a              = x + z;
  b              = y;
}

io::task<void> void_mixed(int& slot, int& out) {
  auto r = co_await io::when_all(note(slot, 9, 2), value(5, 1));
  static_assert(std::is_same_v<decltype(r), std::tuple<int>>);
  out = std::get<0>(r);
}

io::task<void> all_void(int& a, int& b) {
  static_assert(std::is_void_v<decltype(io::when_all(note(a, 1), note(b, 2)).await_resume())>);
  co_await io::when_all(note(a, 1, 3), note(b, 2, 1));
}

io::task<void> env_test(observed* out) {
  auto r = co_await io::when_all(observe(), observe(), observe());
  out[0] = std::get<0>(r);
  out[1] = std::get<1>(r);
  out[2] = std::get<2>(r);
}

io::task<void> order_test(std::vector<int>& log) {
  auto one = [](std::vector<int>& l, int id) -> io::task<void> {
    l.push_back(id);
    co_await post_op{};
    l.push_back(id + 10);
  };
  co_await io::when_all(one(log, 1), one(log, 2), one(log, 3));
}

io::task<void> range_test(std::vector<int>& out) {
  std::vector<io::task<int>> v;
  v.push_back(value(10, 3));
  v.push_back(value(20, 0));
  v.push_back(value(30, 1));
  out = co_await io::when_all(std::move(v));
}

io::task<void> range_void(int* slots) {
  std::vector<io::task<void>> v;
  for (int i = 0; i != 4; ++i)
    v.push_back(note(slots[i], i + 1, i));
  co_await io::when_all(std::move(v));
}

#ifndef TEST_HAS_NO_EXCEPTIONS
io::task<void> failing(int& finished, int& observed_stop, bool& caught_runtime_error, bool& stop_after) {
  const io::io_env* env = co_await io::get_environment;
  try {
    (void)co_await io::when_all(
        until_stopped(finished, observed_stop), boom(2), until_stopped(finished, observed_stop));
  } catch (const std::runtime_error&) {
    caught_runtime_error = true;
  }
  stop_after = env->stop_token.stop_requested(); // the parent's own token is not stopped by a failing child
}

io::task<void> first_of_two(int& finished, int& which) {
  int seen = 0;
  try {
    co_await io::when_all(boom_void(), boom(5), until_stopped(finished, seen));
  } catch (int e) {
    which = e;
  } catch (const std::runtime_error&) {
    which = -1;
  }
}

io::task<void> failing_range(int& finished, int& observed_stop, bool& caught) {
  std::vector<io::task<void>> v;
  v.push_back(until_stopped(finished, observed_stop));
  v.push_back(boom_void());
  v.push_back(until_stopped(finished, observed_stop));
  try {
    co_await io::when_all(std::move(v));
  } catch (int) {
    caught = true;
  }
}
#endif

io::task<int> nested_sum(int depth) {
  if (depth == 0)
    co_return 1;
  auto [a, b] = co_await io::when_all(nested_sum(depth - 1), nested_sum(depth - 1));
  co_return a + b;
}

io::task<void> parent_stop_test(int& finished, int& observed_stop) {
  co_await io::when_all(until_stopped(finished, observed_stop), until_stopped(finished, observed_stop));
}

int main(int, char**) {
  static_assert(io::IoAwaitable<decltype(io::when_all(value(1), value(2)))>);
  static_assert(io::IoAwaitable<decltype(io::when_all(std::vector<io::task<int>>{}))>);
  static_assert(!std::is_copy_constructible_v<decltype(io::when_all(value(1)))>);

  // Values: argument order, void children omitted from the tuple.
  {
    test_context ctx;
    int a = 0;
    std::string b;
    auto t = values_test(a, b);
    run_root(ctx, t);
    assert(a == 4 && b == "two");
  }
  {
    test_context ctx;
    int slot = 0, out = 0;
    auto t = void_mixed(slot, out);
    run_root(ctx, t);
    assert(slot == 9 && out == 5);
  }
  {
    test_context ctx;
    int a = 0, b = 0;
    auto t = all_void(a, b);
    run_root(ctx, t);
    assert(a == 1 && b == 2);
  }

  // Children run concurrently: they start in argument order and interleave on the executor.
  {
    test_context ctx;
    std::vector<int> log;
    auto t = order_test(log);
    run_root(ctx, t);
    assert((log == std::vector<int>{1, 2, 3, 11, 12, 13}));
  }

  // Children get the caller's executor and frame allocator and a stop token of their own, chained to the caller's.
  {
    test_context ctx;
    counting_resource mr;
    observed o[3];
    auto t = env_test(o);
    run_root(ctx, t, {}, &mr);
    for (auto& x : o) {
      assert(x.ctx == &ctx);
      assert(x.stop_possible && !x.stop_requested);
      assert(x.mr == &mr);
    }
    assert(mr.allocations > 0);
  }
  {
    test_context ctx;
    std::stop_source src;
    observed o[3];
    auto t = env_test(o);
    src.request_stop(); // already stopped when the combinator starts
    run_root(ctx, t, src.get_token());
    for (auto& x : o)
      assert(x.stop_requested);
  }

  // The caller's stop request reaches the children.
  {
    test_context ctx;
    std::stop_source src;
    int finished = 0, seen = 0;
    auto t           = parent_stop_test(finished, seen);
    test_executor ex = ctx.get_executor();
    io::io_env env{ex, src.get_token(), nullptr};
    t.handle().promise().set_environment(&env);
    io::continuation start{t.handle()};
    ctx.get_executor().post(start);
    int steps = 0;
    // Run a bounded number of iterations, then stop.
    while (ctx.head && steps++ != 50) {
      io::continuation* c = ctx.head;
      ctx.head            = c->next;
      if (!ctx.head)
        ctx.tail = nullptr;
      c->h.resume();
    }
    assert(finished == 0);
    src.request_stop();
    ctx.run();
    assert(t.handle().done());
    assert(finished == 2 && seen == 2);
  }

  // Range form.
  {
    test_context ctx;
    std::vector<int> out;
    auto t = range_test(out);
    run_root(ctx, t);
    assert((out == std::vector<int>{10, 20, 30}));
  }
  {
    test_context ctx;
    int slots[4] = {};
    auto t       = range_void(slots);
    run_root(ctx, t);
    assert((slots[0] == 1 && slots[1] == 2 && slots[2] == 3 && slots[3] == 4));
  }
#ifndef TEST_HAS_NO_EXCEPTIONS
  {
    bool threw = false;
    try {
      (void)io::when_all(std::vector<io::task<int>>{});
    } catch (const std::invalid_argument&) {
      threw = true;
    }
    assert(threw);
  }

  // A failing child stops its siblings; the exception is rethrown once all children have finished.
  {
    test_context ctx;
    int finished = 0, seen = 0;
    bool caught = false, stop_after = true;
    auto t = failing(finished, seen, caught, stop_after);
    run_root(ctx, t);
    assert(caught);
    assert(finished == 2 && seen == 2);
    assert(!stop_after);
  }
  {
    test_context ctx;
    int finished = 0, which = 0;
    auto t = first_of_two(finished, which);
    run_root(ctx, t);
    assert(which == 17); // boom_void fails first
    assert(finished == 1);
  }
  {
    test_context ctx;
    int finished = 0, seen = 0;
    bool caught = false;
    auto t      = failing_range(finished, seen, caught);
    run_root(ctx, t);
    assert(caught && finished == 2 && seen == 2);
  }
#endif

  // On a thread_pool: nested combinators, children spread over the workers.
  {
    io::thread_pool pool(4);
    std::atomic<int> got{0};
    io::run_async(pool.get_executor(), [&](int v) { got = v; }, [](std::exception_ptr) {})(nested_sum(8));
    pool.join();
    assert(got == 256);
  }
  {
    io::thread_pool pool(4);
    std::atomic<int> total{0};
    for (int i = 0; i != 50; ++i)
      io::run_async(pool.get_executor(), [&](int v) { total += v; }, [](std::exception_ptr) {})(nested_sum(5));
    pool.join();
    assert(total == 50 * 32);
  }

  // Over a strand on a pool, children are serialized with each other.
  {
    io::thread_pool pool(4);
    io::strand s(pool.get_executor());
    std::atomic<int> got{0};
    io::run_async(s, [&](int v) { got = v; }, [](std::exception_ptr) {})(nested_sum(6));
    pool.join();
    assert(got == 64);
  }

  return 0;
}
