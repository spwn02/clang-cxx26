//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23, c++26
// UNSUPPORTED: no-threads
// REQUIRES: linux
// ADDITIONAL_COMPILE_FLAGS: -fexperimental-library

// <io_context>

// An io_context that is destroyed while chains of coroutines are still suspended in it destroys every frame of the
// chains: the root launched by run_async, the tasks below it, the runners of when_all and when_any, the trampoline of
// run(executor)(task) and the drain of a strand. The sentinels count the frames that were destroyed (a leak is also
// reported by the leak checker of the sanitizers, which the tests of the repository run under by hand).

#include <io>
#include <io_context>
#include <io_timer>

#include <atomic>
#include <cassert>
#include <chrono>
#include <coroutine>
#include <memory_resource>
#include <utility>

#include "test_ctx.h"
#include "test_macros.h"

using namespace test_ctx;

struct sentinel {
  std::atomic<int>* n;
  explicit sentinel(std::atomic<int>* p) : n(p) {}
  sentinel(sentinel&& o) noexcept : n(std::exchange(o.n, nullptr)) {}
  ~sentinel() {
    if (n)
      ++*n;
  }
};

// An awaitable that never completes.
struct never {
  bool await_ready() const noexcept { return false; }
  void await_suspend(std::coroutine_handle<>, const io::io_env*) noexcept {}
  void await_resume() const noexcept {}
};

io::task<void> wait_timer(io::io_context& c, sentinel) {
  io::timer t(c, std::chrono::hours(1));
  (void)co_await t.wait();
}

io::task<void> level3(io::io_context& c, std::atomic<int>* d) {
  sentinel s(d);
  io::timer t(c, std::chrono::hours(1));
  (void)co_await t.wait();
}
io::task<void> level2(io::io_context& c, std::atomic<int>* d) {
  sentinel s(d);
  co_await level3(c, d);
}
io::task<void> level1(io::io_context& c, std::atomic<int>* d) {
  sentinel s(d);
  co_await level2(c, d);
}

io::task<void> all_of_timers(io::io_context& c, std::atomic<int>* d) {
  sentinel s(d);
  co_await io::when_all(wait_timer(c, sentinel(d)), wait_timer(c, sentinel(d)), wait_timer(c, sentinel(d)));
}

io::task<void> any_of_timers(io::io_context& c, std::atomic<int>* d) {
  sentinel s(d);
  (void)co_await io::when_any(wait_timer(c, sentinel(d)), wait_timer(c, sentinel(d)));
}

io::task<void> switched(io::io_context& c, io::io_context& other, std::atomic<int>* d) {
  sentinel s(d);
  co_await io::run(other.get_executor())(wait_timer(c, sentinel(d)));
}

io::task<void> switched_same(io::io_context& c, std::atomic<int>* d) {
  sentinel s(d);
  co_await io::run(c.get_executor())(wait_timer(c, sentinel(d)));
}

io::task<void> stuck(std::atomic<int>* d) {
  sentinel s(d);
  co_await never{};
}

static void spawn(io::io_context& c, io::task<void> t) { io::run_async(c.get_executor())(std::move(t)); }

int main(int, char**) {
  // A plain timer wait, started.
  {
    std::atomic<int> d{0};
    {
      io::io_context ctx;
      spawn(ctx, wait_timer(ctx, sentinel(&d)));
      ctx.poll();
      assert(d == 0);
    }
    assert(d == 1);
  }

  // A root that was never started (it waits in the queue of the context).
  {
    std::atomic<int> d{0};
    {
      io::io_context ctx;
      spawn(ctx, wait_timer(ctx, sentinel(&d)));
    }
    assert(d == 1);
  }

  // Three tasks deep.
  {
    std::atomic<int> d{0};
    {
      io::io_context ctx;
      spawn(ctx, level1(ctx, &d));
      ctx.poll();
      assert(d == 0);
    }
    assert(d == 3);
  }

  // The result of a task that finished is not destroyed twice, and a finished root leaves the registry.
  {
    std::atomic<int> d{0};
    {
      io::io_context ctx;
      spawn(ctx, [](std::atomic<int>* d) -> io::task<void> {
        sentinel s(d);
        co_return;
      }(&d));
      ctx.run();
      assert(d == 1);
    }
    assert(d == 1);
  }

  // Several chains, some of them finished.
  {
    std::atomic<int> d{0};
    {
      io::io_context ctx;
      for (int i = 0; i != 20; ++i)
        spawn(ctx, wait_timer(ctx, sentinel(&d)));
      for (int i = 0; i != 20; ++i)
        spawn(ctx, [](std::atomic<int>* d) -> io::task<void> {
          sentinel s(d);
          co_return;
        }(&d));
      ctx.poll();
    }
    assert(d == 40);
  }

  // when_all and when_any of timers, with the runners suspended.
  {
    std::atomic<int> d{0};
    {
      io::io_context ctx;
      spawn(ctx, all_of_timers(ctx, &d));
      spawn(ctx, any_of_timers(ctx, &d));
      ctx.poll();
    }
    assert(d == 1 + 3 + 1 + 2);
  }

  // when_all that was posted but not started.
  {
    std::atomic<int> d{0};
    {
      io::io_context ctx;
      spawn(ctx, all_of_timers(ctx, &d));
      ctx.run_one(); // the root starts and posts the runners
    }
    assert(d == 1 + 3);
  }

  // run(executor)(task) on the same context: the heap trampoline goes with the chain.
  {
    std::atomic<int> d{0};
    {
      io::io_context ctx;
      spawn(ctx, switched_same(ctx, &d));
      ctx.poll();
    }
    assert(d == 2);
  }

  // run(executor)(task) with another context as the executor of the subtask, the timer on the context of the root. The
  // other context outlives the root's and has nothing queued (see the limitation in execution_context::shutdown).
  {
    std::atomic<int> d{0};
    {
      io::io_context other;
      io::io_context ctx;
      spawn(ctx, switched(ctx, other, &d));
      ctx.poll();
      other.poll(); // the trampoline starts the subtask on the other context
      assert(d == 0);
    }
    assert(d == 2);
  }

  // A chain suspended in an awaitable that nobody will ever complete.
  {
    std::atomic<int> d{0};
    {
      io::io_context ctx;
      spawn(ctx, stuck(&d));
      ctx.poll();
    }
    assert(d == 1);
  }

  // The frame allocator sees every frame returned.
  {
    struct counting : std::pmr::memory_resource {
      int allocations = 0, deallocations = 0;
      void* do_allocate(std::size_t n, std::size_t a) override {
        ++allocations;
        return std::pmr::new_delete_resource()->allocate(n, a);
      }
      void do_deallocate(void* p, std::size_t n, std::size_t a) override {
        ++deallocations;
        std::pmr::new_delete_resource()->deallocate(p, n, a);
      }
      bool do_is_equal(const std::pmr::memory_resource& o) const noexcept override { return this == &o; }
    } mr; // declared before the context: it must outlive it
    std::atomic<int> d{0};
    {
      io::io_context ctx;
      ctx.set_frame_allocator(&mr);
      spawn(ctx, level1(ctx, &d));
      spawn(ctx, all_of_timers(ctx, &d));
      ctx.poll();
    }
    assert(mr.allocations != 0 && mr.allocations == mr.deallocations);
  }

  return 0;
}
