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

// std::io::when_any: the first child to complete normally wins, the others are asked to stop and are awaited before the
// call returns, exceptions do not win, the range form, environment propagation, and use on a thread_pool.

#include <io>

#include <atomic>
#include <cassert>
#include <stdexcept>
#include <stop_token>
#include <string>
#include <thread>
#include <variant>
#include <vector>

#include "test_io.h"
#include "test_macros.h"

namespace io = std::io;
using namespace test_io;

io::task<int> value(int v, int hops) {
  for (int i = 0; i != hops; ++i)
    co_await post_op{};
  co_return v;
}

io::task<std::string> text(std::string s, int hops) {
  for (int i = 0; i != hops; ++i)
    co_await post_op{};
  co_return s;
}

io::task<void> delay(int hops, int& slot, int v) {
  for (int i = 0; i != hops; ++i)
    co_await post_op{};
  slot = v;
}

// Hops until stop is requested (or forever-ish); records that it was stopped and that it finished.
struct loser_state {
  int stopped  = 0;
  int finished = 0;
};
io::task<int> loser(loser_state& st, int v) {
  const io::io_env* env = co_await io::get_environment;
  for (int i = 0; i != 100000 && !env->stop_token.stop_requested(); ++i)
    co_await post_op{};
  if (env->stop_token.stop_requested())
    ++st.stopped;
  ++st.finished;
  co_return v;
}

io::task<void> basic(std::size_t& index, int& ival, std::string& sval, loser_state& st) {
  auto r = co_await io::when_any(loser(st, 1), value(7, 2), text("late", 50));
  static_assert(std::is_same_v<decltype(r), std::variant<int, int, std::string>>);
  index = r.index();
  if (r.index() == 1)
    ival = std::get<1>(r);
  if (r.index() == 2)
    sval = std::get<2>(r);
}

// Every other child has finished by the time the call returns.
io::task<void> awaited_before_return(loser_state& st, int& finished_at_return) {
  auto r             = co_await io::when_any(value(1, 1), loser(st, 2), loser(st, 3));
  finished_at_return = st.finished;
  (void)r;
}

io::task<void> with_void(std::size_t& index, bool& is_mono, int& slot) {
  auto r = co_await io::when_any(delay(1, slot, 5), value(1, 30));
  static_assert(std::is_same_v<decltype(r), std::variant<std::monostate, int>>);
  index   = r.index();
  is_mono = std::holds_alternative<std::monostate>(r);
}

io::task<void> same_types(std::size_t& index, int& v) {
  auto r = co_await io::when_any(value(1, 9), value(2, 1), value(3, 20));
  static_assert(std::is_same_v<decltype(r), std::variant<int, int, int>>);
  index = r.index();
  v     = std::visit([](int x) { return x; }, r);
}

io::task<void> range_form(std::size_t& winner, int& v, loser_state& st) {
  std::vector<io::task<int>> tasks;
  tasks.push_back(loser(st, 100));
  tasks.push_back(value(42, 2));
  tasks.push_back(loser(st, 101));
  auto r = co_await io::when_any(std::move(tasks));
  static_assert(std::is_same_v<decltype(r), std::pair<std::size_t, int>>);
  winner = r.first;
  v      = r.second;
}

io::task<void> range_void(std::size_t& winner, int* slots) {
  std::vector<io::task<void>> tasks;
  tasks.push_back(delay(10, slots[0], 1));
  tasks.push_back(delay(3, slots[1], 2));
  tasks.push_back(delay(7, slots[2], 3));
  auto r = co_await io::when_any(std::move(tasks));
  static_assert(std::is_same_v<decltype(r), std::size_t>);
  winner = r;
}

io::task<void> env_test(bool& possible, bool& requested_in_loser, std::pmr::memory_resource*& mr, loser_state& st) {
  auto probe = [](bool& possible, std::pmr::memory_resource*& mr) -> io::task<int> {
    const io::io_env* env = co_await io::get_environment;
    possible              = env->stop_token.stop_possible();
    mr                    = env->frame_allocator;
    co_return 1;
  };
  auto r             = co_await io::when_any(probe(possible, mr), loser(st, 2));
  requested_in_loser = st.stopped == 1;
  (void)r;
}

#ifndef TEST_HAS_NO_EXCEPTIONS
io::task<int> boom(int hops) {
  for (int i = 0; i != hops; ++i)
    co_await post_op{};
  throw std::runtime_error("boom");
  co_return 0;
}

// An exception does not win: the slower normal completion does.
io::task<void> exception_loses(std::size_t& index, int& v) {
  auto r = co_await io::when_any(boom(0), value(5, 6), boom(1));
  index  = r.index();
  v      = std::get<1>(r);
}

// If every child fails, the first exception is rethrown (after all have finished).
io::task<void> all_fail(bool& caught, int& finished) {
  auto counted_boom = [](int hops, int& finished) -> io::task<int> {
    for (int i = 0; i != hops; ++i)
      co_await post_op{};
    ++finished;
    throw std::runtime_error("boom");
    co_return 0;
  };
  try {
    (void)co_await io::when_any(counted_boom(1, finished), counted_boom(5, finished), counted_boom(3, finished));
  } catch (const std::runtime_error&) {
    caught = finished == 3;
  }
}

io::task<void> all_fail_range(bool& caught) {
  std::vector<io::task<int>> tasks;
  tasks.push_back(boom(1));
  tasks.push_back(boom(2));
  try {
    (void)co_await io::when_any(std::move(tasks));
  } catch (const std::runtime_error&) {
    caught = true;
  }
}
#endif

io::task<int> nested(int depth, loser_state& st) {
  if (depth == 0)
    co_return 1;
  auto r = co_await io::when_any(nested(depth - 1, st), loser(st, 0));
  co_return r.index() == 0 ? 1 + std::get<0>(r) : -1000;
}

io::task<int> race_on_pool(std::atomic<int>& finished) {
  auto child = [](int v, int hops, std::atomic<int>& finished) -> io::task<int> {
    const io::io_env* env = co_await io::get_environment;
    for (int i = 0; i != hops && !env->stop_token.stop_requested(); ++i)
      co_await post_op{};
    ++finished;
    co_return v;
  };
  auto r = co_await io::when_any(child(1, 10, finished), child(2, 1000000, finished), child(3, 1000000, finished));
  co_return static_cast<int>(r.index());
}

io::task<void> parent_stopped(loser_state& st) {
  auto r = co_await io::when_any(loser(st, 1), loser(st, 2));
  (void)r;
}

int main(int, char**) {
  static_assert(io::IoAwaitable<decltype(io::when_any(value(1, 0), value(2, 0)))>);

  // First wins; the others are told to stop and are finished when the call returns.
  {
    test_context ctx;
    std::size_t index = 99;
    int ival          = 0;
    std::string sval;
    loser_state st;
    auto t = basic(index, ival, sval, st);
    run_root(ctx, t);
    assert(index == 1 && ival == 7 && sval.empty());
    assert(st.stopped == 1 && st.finished == 1);
  }
  {
    test_context ctx;
    loser_state st;
    int at_return = -1;
    auto t        = awaited_before_return(st, at_return);
    run_root(ctx, t);
    assert(at_return == 2 && st.stopped == 2);
  }

  // A void winner is a monostate alternative; equal types are told apart by position.
  {
    test_context ctx;
    std::size_t index = 99;
    bool mono         = false;
    int slot          = 0;
    auto t            = with_void(index, mono, slot);
    run_root(ctx, t);
    assert(index == 0 && mono && slot == 5);
  }
  {
    test_context ctx;
    std::size_t index = 99;
    int v             = 0;
    auto t            = same_types(index, v);
    run_root(ctx, t);
    assert(index == 1 && v == 2);
  }

  // Range form.
  {
    test_context ctx;
    std::size_t winner = 99;
    int v              = 0;
    loser_state st;
    auto t = range_form(winner, v, st);
    run_root(ctx, t);
    assert(winner == 1 && v == 42 && st.stopped == 2 && st.finished == 2);
  }
  {
    test_context ctx;
    std::size_t winner = 99;
    int slots[3]       = {};
    auto t             = range_void(winner, slots);
    run_root(ctx, t);
    assert(winner == 1);
    assert(slots[1] == 2);
  }
#ifndef TEST_HAS_NO_EXCEPTIONS
  {
    bool threw = false;
    try {
      (void)io::when_any(std::vector<io::task<int>>{});
    } catch (const std::invalid_argument&) {
      threw = true;
    }
    assert(threw);
  }
#endif

  // Environment: own stop token, caller's frame allocator.
  {
    test_context ctx;
    counting_resource mr;
    bool possible = false, loser_stopped = false;
    std::pmr::memory_resource* seen = nullptr;
    loser_state st;
    auto t = env_test(possible, loser_stopped, seen, st);
    run_root(ctx, t, {}, &mr);
    assert(possible && loser_stopped && seen == &mr);
    assert(mr.allocations > 0);
  }

  // The caller's stop request reaches the children (nobody wins until they stop).
  {
    test_context ctx;
    std::stop_source src;
    loser_state st;
    auto t           = parent_stopped(st);
    test_executor ex = ctx.get_executor();
    io::io_env env{ex, src.get_token(), nullptr};
    t.handle().promise().set_environment(&env);
    io::continuation start{t.handle()};
    ctx.get_executor().post(start);
    int steps = 0;
    while (ctx.head && steps++ != 50) {
      io::continuation* c = ctx.head;
      ctx.head            = c->next;
      if (!ctx.head)
        ctx.tail = nullptr;
      c->h.resume();
    }
    assert(st.finished == 0);
    src.request_stop();
    ctx.run();
    assert(t.handle().done());
    assert(st.stopped == 2 && st.finished == 2);
  }

#ifndef TEST_HAS_NO_EXCEPTIONS
  // Exceptions do not win.
  {
    test_context ctx;
    std::size_t index = 99;
    int v             = 0;
    auto t            = exception_loses(index, v);
    run_root(ctx, t);
    assert(index == 1 && v == 5);
  }
  {
    test_context ctx;
    bool caught  = false;
    int finished = 0;
    auto t       = all_fail(caught, finished);
    run_root(ctx, t);
    assert(caught && finished == 3);
  }
  {
    test_context ctx;
    bool caught = false;
    auto t      = all_fail_range(caught);
    run_root(ctx, t);
    assert(caught);
  }
#endif

  // Nested combinators.
  {
    test_context ctx;
    loser_state st;
    int got   = 0;
    auto root = [](int& got, loser_state& st) -> io::task<void> { got = co_await nested(6, st); };
    auto t    = root(got, st);
    run_root(ctx, t);
    assert(got == 7);
  }

  // On a thread_pool: the quick child wins, the others stop early, all three are finished before the result is used.
  {
    io::thread_pool pool(4);
    std::atomic<int> finished{0}, winner{-1}, done{0};
    for (int round = 0; round != 20; ++round) {
      finished = 0;
      done     = 0;
      io::run_async(
          pool.get_executor(),
          [&](int v) {
            winner = v;
            assert(finished == 3);
            done = 1;
          },
          [](std::exception_ptr) {})(race_on_pool(finished));
      while (!done)
        std::this_thread::yield();
      assert(winner == 0);
    }
    pool.join();
  }

  return 0;
}
