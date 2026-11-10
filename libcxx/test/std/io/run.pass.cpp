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

// co_await std::io::run(...)(task): switching the executor, the stop token and the frame allocator for a subtask, and
// inheriting whatever is not switched.

#include <io>

#include <cassert>
#include <stop_token>

#include "test_io.h"
#include "test_macros.h"

namespace io = std::io;
using namespace test_io;

struct observed {
  io::execution_context* ctx;
  bool stop;
  std::pmr::memory_resource* mr;
};

io::task<observed> observe() {
  const io::io_env* env = co_await io::get_environment;
  co_return observed{&env->executor.context(), env->stop_token.stop_requested(), env->frame_allocator};
}

io::task<int> failing() {
#ifndef TEST_HAS_NO_EXCEPTIONS
  throw 3;
#endif
  co_return 3;
}

struct results {
  observed before, child, after;
  int sum = 0;
};

io::task<void> switch_executor(test_context& other, results& r) {
  r.before = co_await observe();
  r.child  = co_await io::run(other.get_executor())(observe());
  r.after  = co_await observe();
}

io::task<void> switch_stop(std::stop_token st, results& r) {
  r.before = co_await observe();
  r.child  = co_await io::run(st)(observe());
  r.after  = co_await observe();
}

io::task<void> switch_allocator(std::pmr::memory_resource* mr, results& r) {
  r.before = co_await observe();
  r.child  = co_await io::run(mr)(observe());
  r.after  = co_await observe();
}

io::task<void> switch_all(test_context& other, std::stop_token st, std::pmr::memory_resource* mr, results& r) {
  r.before = co_await observe();
  r.child  = co_await io::run(other.get_executor(), st, mr)(observe());
  r.after  = co_await observe();
}

io::task<int> value_through_run(test_context& other) {
  int a = co_await io::run(other.get_executor())(io::task<int>(([]() -> io::task<int> { co_return 40; })()));
  int b = co_await io::run(std::stop_token{})(([]() -> io::task<int> { co_return 2; })());
  co_return a + b;
}

io::task<int> exception_through_run(test_context& other, bool switch_ex) {
#ifndef TEST_HAS_NO_EXCEPTIONS
  try {
    if (switch_ex)
      co_await io::run(other.get_executor())(failing());
    else
      co_await io::run(std::stop_token{})(failing());
  } catch (int e) {
    co_return e;
  }
#endif
  co_return 3;
}

// Drives two single-threaded contexts until both are idle.
void run_both(test_context& a, test_context& b) {
  while (a.head || b.head) {
    a.run();
    b.run();
  }
}

template <class T>
void start(test_context& ctx, io::task<T>& t, io::io_env& env, io::continuation& c) {
  t.handle().promise().set_environment(&env);
  c.h = t.handle();
  ctx.get_executor().post(c);
}

int main(int, char**) {
  // Executor switch: the child runs on the other context, the parent resumes on its own.
  {
    test_context a, b;
    results r;
    io::io_env env{a.get_executor(), {}, nullptr};
    io::continuation c;
    auto t = switch_executor(b, r);
    start(a, t, env, c);
    run_both(a, b);
    assert(t.handle().done());
    assert(r.before.ctx == &a);
    assert(r.child.ctx == &b);
    assert(r.after.ctx == &a);
    assert(b.dispatched_queued == 1 && b.posted == 0);
    assert(a.dispatched_inline + a.dispatched_queued == 1); // the parent comes back through a.dispatch
  }
  // Stop token switch: the executor and allocator are inherited.
  {
    test_context a;
    std::stop_source parent_src, child_src;
    child_src.request_stop();
    counting_resource mr;
    results r;
    io::io_env env{a.get_executor(), parent_src.get_token(), &mr};
    io::continuation c;
    io::set_cached_frame_allocator(&mr);
    auto t = switch_stop(child_src.get_token(), r);
    start(a, t, env, c);
    a.run();
    assert(t.handle().done());
    assert(!r.before.stop && r.child.stop && !r.after.stop);
    assert(r.child.ctx == &a);
    assert(r.child.mr == &mr);
    io::set_cached_frame_allocator(nullptr);
  }
  // Frame allocator switch: the child's frames (and below) come from the new resource, the parent's from its own.
  {
    test_context a;
    counting_resource outer, inner;
    results r;
    io::io_env env{a.get_executor(), {}, &outer};
    io::continuation c;
    io::set_cached_frame_allocator(&outer);
    auto t = switch_allocator(&inner, r);
    start(a, t, env, c);
    a.run();
    assert(t.handle().done());
    assert(r.before.mr == &outer && r.child.mr == &inner && r.after.mr == &outer);
    // The slot was restored for the code that follows the call to run().
    assert(io::get_cached_frame_allocator() == &outer);
    assert(inner.allocations == 1);
    // switch_allocator, observe x2 outside; the third observe is inside run()
    assert(outer.allocations == 3);
    io::set_cached_frame_allocator(nullptr);
    t = io::task<void>{};
    assert(outer.deallocations == 3 && inner.deallocations == 1);
  }
  // All three at once.
  {
    test_context a, b;
    counting_resource mr;
    std::stop_source src;
    src.request_stop();
    results r;
    io::io_env env{a.get_executor(), {}, nullptr};
    io::continuation c;
    auto t = switch_all(b, src.get_token(), &mr, r);
    start(a, t, env, c);
    run_both(a, b);
    assert(t.handle().done());
    assert(r.child.ctx == &b && r.child.stop && r.child.mr == &mr);
    assert(r.after.ctx == &a && !r.after.stop && r.after.mr == nullptr);
    assert(mr.allocations == 1 && mr.deallocations == 1);
  }
  // Values and exceptions come back through run().
  {
    test_context a, b;
    io::io_env env{a.get_executor(), {}, nullptr};
    io::continuation c;
    auto t = value_through_run(b);
    start(a, t, env, c);
    run_both(a, b);
    assert(t.handle().promise().result() == 42);
  }
#ifndef TEST_HAS_NO_EXCEPTIONS
  for (bool sw : {false, true}) {
    test_context a, b;
    io::io_env env{a.get_executor(), {}, nullptr};
    io::continuation c;
    auto t = exception_through_run(b, sw);
    start(a, t, env, c);
    run_both(a, b);
    assert(t.handle().promise().result() == 3);
  }
#endif
  return 0;
}
