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

// std::io::thread_pool and std::io::system_context(): workers run posted work, join() waits for counted work, work
// can be posted from many threads, dispatch runs inline on a worker, stop() abandons queued work.

#include <io>

#include <atomic>
#include <cassert>
#include <coroutine>
#include <thread>
#include <utility>
#include <vector>

#include "test_io.h"
#include "test_macros.h"

namespace io = std::io;
using namespace test_io;

io::task<void> bump(std::atomic<int>& n) {
  ++n;
  co_return;
}

io::task<void> record_thread(std::thread::id& id) {
  id = std::this_thread::get_id();
  co_return;
}

// Hops through the executor many times, so that join() must wait for a chain that is mostly suspended.
io::task<void> hopper(std::atomic<int>& n, int hops) {
  for (int i = 0; i != hops; ++i)
    co_await post_op{};
  ++n;
}

// A fire-and-forget coroutine whose frame holds a sentinel.
struct detached {
  struct promise_type {
    detached get_return_object() noexcept { return {std::coroutine_handle<promise_type>::from_promise(*this)}; }
    std::suspend_always initial_suspend() noexcept { return {}; }
    std::suspend_never final_suspend() noexcept { return {}; }
    void return_void() noexcept {}
    void unhandled_exception() noexcept {}
  };
  std::coroutine_handle<> h;
};
struct sentinel {
  std::atomic<int>* destroyed;
  explicit sentinel(std::atomic<int>* d) : destroyed(d) {}
  sentinel(sentinel&& o) noexcept : destroyed(std::exchange(o.destroyed, nullptr)) {}
  ~sentinel() {
    if (destroyed)
      ++*destroyed;
  }
};
// The parameter is copied into the frame, so the sentinel dies with the frame.
detached holds_sentinel(sentinel, std::atomic<int>* ran) {
  ++*ran;
  co_return;
}

int main(int, char**) {
  static_assert(io::ExecutionContext<io::thread_pool>);
  static_assert(io::Executor<io::thread_pool::executor_type>);
  static_assert(!std::is_copy_constructible_v<io::thread_pool>);

  // Tasks run on worker threads, not on the caller.
  {
    io::thread_pool pool(2);
    std::thread::id id;
    io::run_async(pool.get_executor())(record_thread(id));
    pool.join();
    assert(id != std::thread::id());
    assert(id != std::this_thread::get_id());
  }

  // Many tasks launched from many threads.
  {
    constexpr int threads = 8, per_thread = 500;
    std::atomic<int> n{0};
    io::thread_pool pool(4);
    std::vector<std::thread> ts;
    for (int t = 0; t != threads; ++t)
      ts.emplace_back([&] {
        for (int i = 0; i != per_thread; ++i)
          io::run_async(pool.get_executor())(bump(n));
      });
    for (auto& t : ts)
      t.join();
    pool.join();
    assert(n == threads * per_thread);
  }

  // join() waits for counted work that keeps re-posting itself.
  {
    std::atomic<int> n{0};
    io::thread_pool pool(3);
    for (int i = 0; i != 20; ++i)
      io::run_async(pool.get_executor())(hopper(n, 200));
    pool.join();
    assert(n == 20);
  }

  // Executor properties: equality, context, dispatch from a non-worker thread posts.
  {
    io::thread_pool pool(1);
    io::thread_pool other(1);
    auto ex = pool.get_executor();
    assert(ex == pool.get_executor());
    assert(!(ex == other.get_executor()));
    assert(&ex.context() == &pool);

    std::atomic<int> destroyed{0}, ran{0};
    detached d = holds_sentinel(sentinel(&destroyed), &ran);
    io::continuation c{d.h};
    std::coroutine_handle<> r = ex.dispatch(c);
    assert(r.address() == std::noop_coroutine().address());
    pool.stop();
    // stop() was racing with the worker: either it ran the item or it is abandoned at destruction.
  }

  // Queued work that never ran is destroyed (not resumed) with the pool, after stop().
  {
    std::atomic<int> destroyed{0}, ran{0};
    {
      io::thread_pool pool(1);
      pool.stop();
      detached d = holds_sentinel(sentinel(&destroyed), &ran);
      io::continuation c{d.h};
      pool.get_executor().post(c);
    }
    assert(ran == 0);
    assert(destroyed == 1);
  }

  // join() with no work returns at once and join() is idempotent.
  {
    io::thread_pool pool(2);
    pool.join();
    pool.join();
  }

  // The system context is one object and executes work.
  {
    io::thread_pool& a = io::system_context();
    io::thread_pool& b = io::system_context();
    assert(&a == &b);
    std::atomic<int> done{0};
    std::thread::id id;
    io::run_async(a.get_executor(), [&] {
      done = 1;
      done.notify_all();
    }, [](std::exception_ptr) {})(record_thread(id));
    done.wait(0);
    assert(id != std::this_thread::get_id());
  }

  return 0;
}
