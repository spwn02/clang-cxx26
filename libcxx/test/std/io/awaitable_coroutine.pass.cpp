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

// A hand-written IoAwaitable coroutine chain: the environment reaches every awaitable through the two-argument
// await_suspend, the executor resumes coroutines, the stop token propagates, and frames come from the frame allocator
// delivered through the out-of-band slot.

#include <io>

#include <cassert>
#include <coroutine>
#include <stop_token>

#include "test_io.h"

namespace io = std::io;
using namespace test_io;

task<int> leaf() { co_return 7; }

task<int> middle() {
  co_await dispatch_op{}; // resumes inline through symmetric transfer
  int v = co_await leaf();
  co_return v + 1;
}

task<void> root(int& out) {
  out = co_await middle();
  co_await post_op{}; // always deferred
  out += co_await middle();
}

task<bool> stop_probe() {
  bool b = co_await stop_query_op{};
  co_return b;
}

task<int> thrower() {
#ifndef TEST_HAS_NO_EXCEPTIONS
  throw 5;
#endif
  co_return 0;
}

task<int> catcher() {
#ifndef TEST_HAS_NO_EXCEPTIONS
  try {
    co_await thrower();
  } catch (int e) {
    co_return e;
  }
#endif
  co_return 5;
}

int main(int, char**) {
  // Results flow through the chain; dispatch inlines, post defers.
  {
    test_context ctx;
    int out      = 0;
    task<void> t = root(out);
    run_to_completion(ctx, t);
    assert(out == 16);
    assert(ctx.posted == 2); // the start, and post_op
    assert(ctx.dispatched_inline == 2);
    assert(ctx.dispatched_queued == 0);
  }

  // The stop token reaches the leaf awaitable through the environment.
  {
    test_context ctx;
    task<bool> t = stop_probe();
    run_to_completion(ctx, t);
    assert(t.handle().promise().result() == false);
  }
  {
    test_context ctx;
    std::stop_source src;
    src.request_stop();
    task<bool> t = stop_probe();
    run_to_completion(ctx, t, src.get_token());
    assert(t.handle().promise().result() == true);
  }

  // Exceptions propagate through co_await.
  {
    test_context ctx;
    task<int> t = catcher();
    run_to_completion(ctx, t);
    assert(t.handle().promise().result() == 5);
  }

  // Frames are allocated from the allocator that the slot carries when the coroutine is called, and the chain keeps
  // using the environment's allocator.
  {
    test_context ctx;
    counting_resource mr;
    io::set_cached_frame_allocator(&mr);
    int out      = 0;
    task<void> t = root(out); // allocated from mr
    assert(mr.allocations == 1);
    run_to_completion(ctx, t, {}, &mr);
    // root, 2x middle, 2x leaf
    assert(mr.allocations == 5);
    assert(out == 16);
    io::set_cached_frame_allocator(nullptr);
  }
  // Every frame is returned to the resource it came from, whatever the slot holds at destruction.
  {
    test_context ctx;
    counting_resource mr;
    {
      io::set_cached_frame_allocator(&mr);
      int out      = 0;
      task<void> t = root(out);
      run_to_completion(ctx, t, {}, &mr);
      io::set_cached_frame_allocator(nullptr);
    }
    assert(mr.allocations == mr.deallocations);
    assert(mr.allocations == 5);
  }

  // The context's default frame allocator is what a launch function would put into the environment.
  {
    test_context ctx;
    counting_resource mr;
    ctx.set_frame_allocator(&mr);
    io::io_env env{ctx.get_executor(), {}, ctx.get_frame_allocator()};
    assert(env.frame_allocator == &mr);
    assert(&env.executor.context() == &ctx);
  }

  // io_env defaults: no executor reference payload, no stop state, no allocator.
  {
    io::io_env env;
    assert(env.frame_allocator == nullptr);
    assert(!env.stop_token.stop_possible());
  }

  return 0;
}
