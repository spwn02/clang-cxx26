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

// std::io::any_executor: owning type-erased Executor; copy, compare, post, dispatch, target, and use as the executor of
// a launch function and of a strand.

#include <io>

#include <atomic>
#include <cassert>
#include <type_traits>

#include "test_io.h"
#include "test_macros.h"

namespace io = std::io;
using namespace test_io;

static_assert(io::Executor<io::any_executor>);
static_assert(std::is_nothrow_copy_constructible_v<io::any_executor>);
static_assert(std::is_nothrow_default_constructible_v<io::any_executor>);
static_assert(std::is_convertible_v<test_executor, io::any_executor>);
static_assert(std::is_convertible_v<io::thread_pool::executor_type, io::any_executor>);
static_assert(!std::is_convertible_v<int, io::any_executor>);
// An any_executor can be viewed through an executor_ref.
static_assert(std::is_constructible_v<io::executor_ref, const io::any_executor&>);

io::task<int> answer() { co_return 42; }

int main(int, char**) {
  // Empty state.
  {
    io::any_executor e;
    assert(!e);
    assert(e == io::any_executor());
    assert(e.target<test_executor>() == nullptr);
  }

  // Construction from an executor; the stored copy outlives the source object.
  {
    test_context ctx;
    io::any_executor e;
    {
      test_executor te = ctx.get_executor();
      e                = te;
    }
    assert(e);
    assert(&e.context() == &ctx);
    assert(e.target<test_executor>() != nullptr);
    assert(e.target<test_executor>()->ctx == &ctx);
    assert(e.target<io::thread_pool::executor_type>() == nullptr);

    e.on_work_started();
    e.on_work_finished();
    assert(ctx.work_started == 1 && ctx.work_finished == 1);

    // post always queues; dispatch follows the wrapped executor.
    bool ran = false;
    auto t   = [](bool& r) -> io::task<void> {
      r = true;
      co_return;
    }(ran);
    io::io_env env{e, {}, nullptr};
    t.handle().promise().set_environment(&env);
    io::continuation c{t.handle()};
    e.post(c);
    assert(ctx.posted == 1 && !ran);
    ctx.run();
    assert(ran);

    io::continuation d{std::noop_coroutine()};
    auto h = e.dispatch(d); // not running: queued
    assert(h == std::noop_coroutine());
    assert(ctx.dispatched_queued == 1);
  }

  // Copies compare equal and share the stored executor; different executors and different types do not compare equal.
  {
    test_context a, b;
    io::any_executor ea(a.get_executor());
    io::any_executor ea2 = ea;
    io::any_executor eb(b.get_executor());
    io::thread_pool pool(1);
    io::any_executor ep(pool.get_executor());
    assert(ea == ea2);
    assert(!(ea == eb));
    assert(!(ea == ep));
    assert(ea != eb);
    assert(!(ea == io::any_executor()));
    assert(ea.target<test_executor>() == ea2.target<test_executor>());
    ea2 = eb;
    assert(ea2 == eb && !(ea2 == ea));
    assert(ep == io::any_executor(pool.get_executor()));
    pool.join();
  }

  // As the executor of run_async, over a thread_pool.
  {
    io::thread_pool pool(2);
    std::atomic<int> got{0};
    io::any_executor e(pool.get_executor());
    io::run_async(e, [&](int v) { got = v; }, [](std::exception_ptr) {})(answer());
    pool.join();
    assert(got == 42);
  }

  // As the inner executor of a strand, and as a strand wrapped by an any_executor.
  {
    test_context ctx;
    io::strand s(io::any_executor(ctx.get_executor()));
    int got = 0;
    io::run_async(s, [&](int v) { got = v; }, [](std::exception_ptr) {})(answer());
    ctx.run();
    assert(got == 42);

    io::any_executor es(s);
    assert(es.target<io::strand<io::any_executor>>() != nullptr);
    assert(es == io::any_executor(s));
    io::strand other(io::any_executor(ctx.get_executor()));
    assert(!(es == io::any_executor(other)));
    got = 0;
    io::run_async(es, [&](int v) { got = v; }, [](std::exception_ptr) {})(answer());
    ctx.run();
    assert(got == 42);
  }

  // The reference view works through the type-erased executor.
  {
    test_context ctx;
    io::any_executor e(ctx.get_executor());
    io::executor_ref r(e);
    assert(&r.context() == &ctx);
  }

  return 0;
}
