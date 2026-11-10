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

// std::io::strand<Ex>: serializes the continuations that go through it, over thread_pool and over any other Executor;
// dispatch runs inline inside the strand and queues outside of it.

#include <io>

#include <atomic>
#include <cassert>
#include <thread>
#include <type_traits>
#include <vector>

#include "test_io.h"
#include "test_macros.h"

namespace io = std::io;
using namespace test_io;

static_assert(io::Executor<io::strand<io::thread_pool::executor_type>>);
static_assert(io::Executor<io::strand<test_executor>>);
static_assert(io::Executor<io::strand<io::any_executor>>);
static_assert(std::is_copy_constructible_v<io::strand<test_executor>>);
static_assert(std::is_same_v<io::strand<test_executor>::inner_executor_type, test_executor>);

// Increments a plain counter with a read-modify-write that is only correct if no two bodies overlap.
struct unsafe_counter {
  int value = 0;
  std::atomic<int> inside{0};
  void bump() {
    assert(inside.fetch_add(1) == 0);
    int v = value;
    std::this_thread::yield();
    value = v + 1;
    assert(inside.fetch_sub(1) == 1);
  }
};

io::task<void> worker(unsafe_counter& c, int rounds) {
  for (int i = 0; i != rounds; ++i) {
    co_await post_op{}; // resumes through the strand's executor
    c.bump();
  }
}

io::task<void> via_dispatch(unsafe_counter& c, int rounds) {
  for (int i = 0; i != rounds; ++i) {
    co_await dispatch_op{};
    c.bump();
  }
}

int main(int, char**) {
  // Many tasks on a multi-threaded pool; the counter has no protection but the strand.
  {
    constexpr int tasks = 16, rounds = 200;
    unsafe_counter c;
    io::thread_pool pool(4);
    io::strand s(pool.get_executor());
    for (int i = 0; i != tasks; ++i)
      io::run_async(s)(worker(c, rounds));
    pool.join();
    assert(c.value == tasks * rounds);
  }

  // The same through dispatch, with several posting threads.
  {
    constexpr int threads = 4, tasks = 8, rounds = 200;
    unsafe_counter c;
    io::thread_pool pool(4);
    io::strand s(pool.get_executor());
    std::vector<std::thread> ts;
    for (int t = 0; t != threads; ++t)
      ts.emplace_back([&] {
        for (int i = 0; i != tasks; ++i)
          io::run_async(s)(via_dispatch(c, rounds));
      });
    for (auto& t : ts)
      t.join();
    pool.join();
    assert(c.value == threads * tasks * rounds);
  }

  // Copies share the serialization state; distinct strands do not compare equal.
  {
    io::thread_pool pool(2);
    io::strand a(pool.get_executor());
    io::strand b = a;
    io::strand c(pool.get_executor());
    assert(a == b);
    assert(!(a == c));
    assert(a.get_inner_executor() == pool.get_executor());
    assert(&a.context() == &pool);
    unsafe_counter counter;
    for (int i = 0; i != 8; ++i) {
      io::run_async(a)(worker(counter, 100));
      io::run_async(b)(worker(counter, 100));
    }
    pool.join();
    assert(counter.value == 1600);
  }

  // Over an executor that is not a thread_pool: post queues on the strand, which posts one drain to the inner executor.
  {
    test_context ctx;
    io::strand s(ctx.get_executor());
    int order[3] = {};
    int n        = 0;
    io::continuation c1, c2, c3;
    auto make = [&](int* slot) {
      return [](int* slot, int* n) -> io::task<void> {
        *slot = ++*n;
        co_return;
      }(slot, &n);
    };
    auto t1          = make(&order[0]);
    auto t2          = make(&order[1]);
    auto t3          = make(&order[2]);
    test_executor ex = ctx.get_executor();
    io::io_env env{ex, {}, nullptr};
    t1.handle().promise().set_environment(&env);
    t2.handle().promise().set_environment(&env);
    t3.handle().promise().set_environment(&env);
    c1.h = t1.handle();
    c2.h = t2.handle();
    c3.h = t3.handle();
    assert(!s.running_in_this_thread());
    s.post(c1);
    s.post(c2);
    assert(ctx.posted == 1); // one drain for both
    s.post(c3);
    assert(ctx.posted == 1);
    assert(n == 0);
    ctx.run();
    assert(order[0] == 1 && order[1] == 2 && order[2] == 3);
    assert(t1.handle().done() && t2.handle().done() && t3.handle().done());
    assert(!s.running_in_this_thread());
    // Idle again: the next post schedules a new drain.
    io::continuation c4;
    auto t4 = make(&order[0]);
    t4.handle().promise().set_environment(&env);
    c4.h = t4.handle();
    s.post(c4);
    assert(ctx.posted == 2);
    ctx.run();
    assert(t4.handle().done());
  }

  // dispatch: queued from outside the strand, inline inside it.
  {
    test_context ctx;
    io::strand s(ctx.get_executor());
    struct probe {
      io::strand<test_executor>* s;
      bool inside_before = false, inside_after = false, was_inline = false;
      io::continuation cont{};
    } p{&s, false, false, false, {}};

    struct inner_awaiter {
      probe* p;
      bool await_ready() const noexcept { return false; }
      std::coroutine_handle<> await_suspend(std::coroutine_handle<> h, const io::io_env*) {
        p->inside_after = p->s->running_in_this_thread();
        p->cont.h       = h;
        auto r          = p->s->dispatch(p->cont);
        p->was_inline   = r == h;
        return r;
      }
      void await_resume() const noexcept {}
    };
    auto body = [](probe* p) -> io::task<void> {
      p->inside_before = p->s->running_in_this_thread();
      co_await inner_awaiter{p};
    };
    auto t = body(&p);
    io::io_env env{s, {}, nullptr};
    t.handle().promise().set_environment(&env);
    io::continuation start{t.handle()};
    // Outside the strand dispatch queues and returns noop.
    auto r = s.dispatch(start);
    assert(r == std::noop_coroutine());
    assert(!t.handle().done());
    ctx.run();
    assert(t.handle().done());
    assert(p.inside_before && p.inside_after && p.was_inline);
  }

  // A strand is an Executor for run_async: work tracking passes through to the inner executor.
  {
    test_context ctx;
    io::strand s(ctx.get_executor());
    s.on_work_started();
    s.on_work_finished();
    assert(ctx.work_started == 1 && ctx.work_finished == 1);
  }

  // Continuations still queued when the last owner of the strand goes away are destroyed, not leaked.
  {
    test_context ctx;
    counting_resource mr;
    {
      auto make = []() -> io::task<void> { co_return; };
      io::set_cached_frame_allocator(&mr);
      auto t = make();
      io::set_cached_frame_allocator(nullptr);
      io::continuation c{t.release()}; // declared before the strand: it must outlive the queue
      io::strand s(ctx.get_executor());
      s.post(c); // the drain is posted to the context, which never runs it
      assert(ctx.posted == 1);
      // Dropping the context's queue abandons the drain coroutine; the strand state and the queued task go when the
      // strand does.
      io::continuation* d = ctx.head;
      ctx.head = ctx.tail = nullptr;
      d->h.destroy();
      assert(mr.allocations == 1 && mr.deallocations == 0);
    }
    assert(mr.allocations == 1 && mr.deallocations == 1);
  }

  return 0;
}
