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

// std::io::task<T>: values, void, exceptions, nesting, symmetric transfer, the frame allocator footer, and the
// environment queries (P4100R1 8.2).

#include <io>

#include <cassert>
#include <coroutine>
#include <stop_token>

#include "test_io.h"
#include "test_macros.h"

namespace io = std::io;
using namespace test_io;

// Drives a root task on a test_context: starts it through the executor with the given environment.
template <class T>
void drive(test_context& ctx, io::task<T>& t, std::stop_token st = {}, std::pmr::memory_resource* mr = nullptr) {
  auto env_executor = ctx.get_executor(); // io_env keeps a reference: it must name an object that outlives it
  io::io_env env{env_executor, st, mr};
  t.handle().promise().set_environment(&env);
  io::continuation start{t.handle()};
  ctx.get_executor().post(start);
  ctx.run();
  assert(t.handle().done());
}

io::task<int> value() { co_return 42; }
io::task<void> nothing() { co_return; }
io::task<int> add(int a, int b) { co_return a + b; }

io::task<int> nested() {
  int x = co_await add(1, 2);
  int y = co_await add(x, co_await value());
  co_return y;
}

io::task<int> thrower() {
#ifndef TEST_HAS_NO_EXCEPTIONS
  throw 17;
#endif
  co_return 0;
}

io::task<int> catcher() {
#ifndef TEST_HAS_NO_EXCEPTIONS
  try {
    co_await thrower();
  } catch (int e) {
    co_return e;
  }
#endif
  co_return 17;
}

struct move_only {
  int v;
  explicit move_only(int i) : v(i) {}
  move_only(move_only&&) = default;
};
io::task<move_only> make_move_only() { co_return move_only(9); }
io::task<int> use_move_only() {
  move_only m = co_await make_move_only();
  co_return m.v;
}

// Symmetric transfer: a million synchronous child awaits in a loop, and a deep chain of nested tasks.
io::task<int> one() { co_return 1; }
io::task<long> loop(int n) {
  long sum = 0;
  for (int i = 0; i != n; ++i)
    sum += co_await one();
  co_return sum;
}
io::task<int> deep(int n) {
  if (n == 0)
    co_return 0;
  co_return 1 + co_await deep(n - 1);
}

io::task<bool> stop_requested() {
  std::stop_token st = co_await io::get_stop_token;
  co_return st.stop_requested();
}
io::task<const io::io_env*> environment() { co_return co_await io::get_environment; }

io::task<int> uses_awaitable_lvalue() {
  io::task<int> child = add(20, 22);
  int r               = co_await child; // lvalue awaitable
  co_return r;
}

int main(int, char**) {
  static_assert(io::IoAwaitable<io::task<int>>);
  static_assert(io::IoAwaitable<io::task<void>>);
  static_assert(io::IoRunnable<io::task<int>>);
  static_assert(io::IoRunnable<io::task<void>>);
  static_assert(io::IoRunnable<io::task<move_only>>);
  static_assert(!std::is_copy_constructible_v<io::task<int>>);
  static_assert(std::is_nothrow_move_constructible_v<io::task<int>>);

  {
    test_context ctx;
    auto t = value();
    drive(ctx, t);
    assert(t.handle().promise().result() == 42);
  }
  {
    test_context ctx;
    auto t = nothing();
    drive(ctx, t);
  }
  {
    test_context ctx;
    auto t = nested();
    drive(ctx, t);
    assert(t.handle().promise().result() == 45);
  }
  {
    test_context ctx;
    auto t = uses_awaitable_lvalue();
    drive(ctx, t);
    assert(t.handle().promise().result() == 42);
  }
  {
    test_context ctx;
    auto t = use_move_only();
    drive(ctx, t);
    assert(t.handle().promise().result() == 9);
  }
  {
    test_context ctx;
    auto t = catcher();
    drive(ctx, t);
    assert(t.handle().promise().result() == 17);
  }
#ifndef TEST_HAS_NO_EXCEPTIONS
  {
    test_context ctx;
    auto t = thrower();
    drive(ctx, t);
    assert(t.handle().promise().exception());
    try {
      std::rethrow_exception(t.handle().promise().exception());
      assert(false);
    } catch (int e) {
      assert(e == 17);
    }
  }
#endif
  {
    test_context ctx;
    auto t = loop(1'000'000);
    drive(ctx, t);
    assert(t.handle().promise().result() == 1'000'000);
  }
  {
    test_context ctx;
    auto t = deep(100'000);
    drive(ctx, t);
    assert(t.handle().promise().result() == 100'000);
  }

  // Environment queries.
  {
    test_context ctx;
    auto t = stop_requested();
    drive(ctx, t);
    assert(!t.handle().promise().result());
  }
  {
    test_context ctx;
    std::stop_source src;
    src.request_stop();
    auto t = stop_requested();
    drive(ctx, t, src.get_token());
    assert(t.handle().promise().result());
  }
  {
    test_context ctx;
    auto t = environment();
    drive(ctx, t);
    assert(t.handle().promise().result() != nullptr);
  }

  // The frame allocator: every frame of a chain comes from the resource and returns to it.
  {
    test_context ctx;
    counting_resource mr;
    io::set_cached_frame_allocator(&mr);
    {
      auto t = nested(); // allocated from mr
      assert(mr.allocations == 1);
      drive(ctx, t, {}, &mr);
      assert(t.handle().promise().result() == 45);
      // nested + add + value + add
      assert(mr.allocations == 4);
    }
    assert(mr.deallocations == 4);
    io::set_cached_frame_allocator(nullptr);
  }
  // Frames are freed to the resource they came from even if the slot changed.
  {
    counting_resource mr;
    io::set_cached_frame_allocator(&mr);
    auto t = value();
    io::set_cached_frame_allocator(nullptr);
    assert(mr.allocations == 1);
    t = io::task<int>{};
    assert(mr.deallocations == 1);
  }

  return 0;
}
