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

// executor_ref (type erasure of an Executor) and continuation queueing (P4003R3 4.3).

#include <io>

#include <cassert>
#include <coroutine>
#include <type_traits>

#include "test_io.h"

namespace io = std::io;
using namespace test_io;

static_assert(sizeof(io::executor_ref) == 2 * sizeof(void*));
static_assert(std::is_nothrow_copy_constructible_v<io::executor_ref>);
static_assert(std::is_nothrow_default_constructible_v<io::executor_ref>);
static_assert(std::is_nothrow_constructible_v<io::executor_ref, const test_executor&>);
static_assert(std::is_aggregate_v<io::continuation>);
static_assert(std::is_same_v<decltype(io::continuation::h), std::coroutine_handle<>>);
static_assert(std::is_same_v<decltype(io::continuation::next), io::continuation*>);
static_assert(std::is_same_v<decltype(std::declval<const io::executor_ref&>().context()), io::execution_context&>);
static_assert(noexcept(std::declval<const io::executor_ref&>().context()));
static_assert(
    std::is_same_v<decltype(std::declval<const io::executor_ref&>().dispatch(std::declval<io::continuation&>())),
                   std::coroutine_handle<>>);
static_assert(
    std::is_void_v<decltype(std::declval<const io::executor_ref&>().post(std::declval<io::continuation&>()))>);

int main(int, char**) {
  {
    io::continuation c;
    assert(!c.h);
    assert(c.next == nullptr);
  }

  test_context ctx;
  test_executor ex = ctx.get_executor();
  assert(ex == ctx.get_executor());
  io::executor_ref ref(ex);

  // context() returns the context of the referenced executor.
  assert(&ref.context() == &ctx);

  // post always defers, in FIFO order, without allocating a node: the continuations live on the stack.
  io::continuation a{std::noop_coroutine()};
  io::continuation b{std::noop_coroutine()};
  ref.post(a);
  ref.post(b);
  assert(ctx.posted == 2);
  assert(ctx.head == &a);
  assert(a.next == &b);
  assert(b.next == nullptr);
  assert(ctx.tail == &b);

  // dispatch outside the run loop queues and returns noop_coroutine().
  io::continuation c{std::noop_coroutine()};
  std::coroutine_handle<> r = ref.dispatch(c);
  assert(ctx.dispatched_queued == 1);
  assert(r.address() == std::noop_coroutine().address());
  assert(b.next == &c);
  ctx.run();
  assert(ctx.head == nullptr && ctx.tail == nullptr);

  // dispatch inside the run loop hands the handle back for symmetric transfer.
  ctx.running                  = true;
  std::coroutine_handle<> mine = std::noop_coroutine();
  io::continuation d{mine};
  std::coroutine_handle<> r2 = ref.dispatch(d);
  assert(r2.address() == mine.address());
  assert(ctx.dispatched_inline == 1);
  assert(ctx.head == nullptr);
  ctx.running = false;

  // The reference does not own the executor, and copies refer to the same executor.
  io::executor_ref copy = ref;
  assert(&copy.context() == &ctx);

  // Two contexts: each reference reaches its own.
  test_context ctx2;
  test_executor ex2 = ctx2.get_executor();
  io::executor_ref ref2(ex2);
  assert(&ref2.context() == &ctx2);
  io::continuation e{std::noop_coroutine()};
  ref2.post(e);
  assert(ctx2.posted == 1 && ctx.posted == 2);
  ctx2.run();

  // A default-constructed reference is empty (it refers to nothing and is only a placeholder).
  io::executor_ref empty;
  (void)empty;

  return 0;
}
