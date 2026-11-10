//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// REQUIRES: std-at-least-c++29
// UNSUPPORTED: no-threads
// ADDITIONAL_COMPILE_FLAGS: -fexperimental-library

// <io_bridge>

// io::task -> as_sender -> await_sender -> io::task: values, error codes, exceptions and cancellation survive the round
// trip.

#include <io_bridge>

#include <string>

#include "test_bridge.h"

using namespace test_bridge;

io::task<int> produce(int v) {
  co_await test_io::post_op{};
  co_return v;
}
io::task<std::error_code> fail_with(std::errc e) {
  co_await test_io::post_op{};
  co_return std::make_error_code(e);
}
io::task<void> boom() {
  co_await test_io::post_op{};
  TEST_THROW(5);
}

// await_sender(as_sender(task)) needs the receiver's environment: write_env supplies the io environment of the awaiting
// coroutine, so the round trip is a pure sender expression.
io::task<int> round_value(io::io_sender_env env) {
  co_return co_await io::await_sender(ex::write_env(io::as_sender(produce(41)), env) | ex::then([](int v) { return v + 1; }));
}
io::task<io::io_result<>> round_ec(io::io_sender_env env, std::errc e) {
  co_return co_await io::await_sender(ex::write_env(io::as_sender(fail_with(e)), env));
}
#ifndef TEST_HAS_NO_EXCEPTIONS
io::task<int> round_throw(io::io_sender_env env) {
  try {
    co_await io::await_sender(ex::write_env(io::as_sender(boom()), env));
  } catch (int e) {
    co_return e;
  }
  co_return 0;
}
io::task<int> round_stop(io::io_sender_env env) {
  try {
    co_await io::await_sender(ex::write_env(io::as_sender([]() -> io::task<void> { co_await wait_for_stop{}; }()), env));
  } catch (const std::system_error& e) {
    co_return e.code() == io::cond::canceled ? 1 : 2;
  }
  co_return 0;
}
#endif

int main(int, char**) {
  io::thread_pool pool(2);
  auto exe = pool.get_executor();
  io::io_sender_env env{io::executor_ref(exe), {}};

  assert(run_on_pool(pool, round_value(env)) == 42);
  {
    auto [ec] = run_on_pool(pool, round_ec(env, std::errc::connection_refused));
    assert(ec == std::errc::connection_refused);
  }
  {
    auto [ec] = run_on_pool(pool, round_ec(env, std::errc{}));
    assert(!ec);
  }
#ifndef TEST_HAS_NO_EXCEPTIONS
  assert(run_on_pool(pool, round_throw(env)) == 5);
#endif
#ifndef TEST_HAS_NO_EXCEPTIONS
  // cancellation: the stop token of the env is triggered while the inner task waits
  {
    std::stop_source src;
    io::io_sender_env stoppable{io::executor_ref(exe), src.get_token()};
    std::promise<int> p;
    auto f = p.get_future();
    io::run_async(exe, [&](int v) { p.set_value(v); }, [&](std::exception_ptr) { p.set_value(-1); })(round_stop(stoppable));
    src.request_stop();
    assert(f.get() == 1);
  }
#endif
  pool.join();
  return 0;
}
