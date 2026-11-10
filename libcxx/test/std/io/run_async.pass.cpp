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

// std::io::run_async: two-phase launch with and without handlers, stop token and frame allocator arguments, the
// environment form, and work counting.

#include <io>

#include <cassert>
#include <exception>
#include <stop_token>
#include <string>

#include "test_io.h"
#include "test_macros.h"

namespace io = std::io;
using namespace test_io;

io::task<int> answer() { co_return 42; }
io::task<void> nothing() { co_return; }
io::task<std::string> text() { co_return std::string("a string long enough to avoid the small buffer optimization"); }
io::task<int> fail() {
#ifndef TEST_HAS_NO_EXCEPTIONS
  throw 7;
#endif
  co_return 7;
}
io::task<bool> stop_state() { co_return (co_await io::get_stop_token).stop_requested(); }
io::task<int> posting() {
  co_await post_op{};
  co_return 5;
}

int main(int, char**) {
  // Fire and forget: nothing runs before the executor runs, then the task completes and the work count is balanced.
  {
    test_context ctx;
    io::run_async(ctx.get_executor())(answer());
    assert(ctx.posted == 1);
    assert(ctx.work_started == 1 && ctx.work_finished == 0);
    ctx.run();
    assert(ctx.work_started == 1 && ctx.work_finished == 1);
  }
  {
    test_context ctx;
    io::run_async(ctx.get_executor())(nothing());
    ctx.run();
    assert(ctx.work_finished == 1);
  }

  // Handlers receive the value or the exception.
  {
    test_context ctx;
    int got = 0;
    bool err = false;
    io::run_async(ctx.get_executor(), [&](int v) { got = v; }, [&](std::exception_ptr) { err = true; })(answer());
    ctx.run();
    assert(got == 42 && !err);
  }
  {
    test_context ctx;
    std::string got;
    io::run_async(ctx.get_executor(), [&](std::string&& s) { got = std::move(s); }, [](std::exception_ptr) {})(text());
    ctx.run();
    assert(got.size() > 30);
  }
  {
    test_context ctx;
    bool called = false;
    io::run_async(ctx.get_executor(), [&] { called = true; }, [](std::exception_ptr) {})(nothing());
    ctx.run();
    assert(called);
  }
#ifndef TEST_HAS_NO_EXCEPTIONS
  {
    test_context ctx;
    int got = 0;
    bool value = false;
    io::run_async(ctx.get_executor(), [&](int) { value = true; }, [&](std::exception_ptr ep) {
      try {
        std::rethrow_exception(ep);
      } catch (int e) {
        got = e;
      }
    })(fail());
    ctx.run();
    assert(got == 7 && !value);
    assert(ctx.work_finished == 1);
  }
  // Without handlers the exception is rethrown out of resume(), on the executor's thread. (safe_resume is noexcept, so
  // an executor that resumes through it terminates the program; this test resumes the handle directly.)
  {
    test_context ctx;
    io::run_async(ctx.get_executor())(fail());
    bool caught = false;
    try {
      ctx.head->h.resume();
    } catch (int e) {
      caught = e == 7;
    }
    assert(caught);
  }
#endif
  // A single handler serves both outcomes.
  {
    test_context ctx;
    int got = 0;
    io::run_async(ctx.get_executor(), [&](auto&&... a) {
      if constexpr (sizeof...(a) == 1 && std::is_same_v<std::remove_cvref_t<decltype((a, ...))>, int>)
        got = (a, ...);
    })(answer());
    ctx.run();
    assert(got == 42);
  }

  // Chains that suspend still complete and are counted until the end.
  {
    test_context ctx;
    int got = 0;
    io::run_async(ctx.get_executor(), [&](int v) { got = v; }, [](std::exception_ptr) {})(posting());
    ctx.run();
    assert(got == 5 && ctx.work_finished == 1);
  }

  // Stop token argument.
  {
    test_context ctx;
    std::stop_source src;
    bool seen = false;
    io::run_async(ctx.get_executor(), src.get_token(), [&](bool b) { seen = b; }, [](std::exception_ptr) {})(stop_state());
    src.request_stop();
    ctx.run();
    assert(seen);
  }
  {
    test_context ctx;
    bool seen = true;
    io::run_async(ctx.get_executor(), [&](bool b) { seen = b; }, [](std::exception_ptr) {})(stop_state());
    ctx.run();
    assert(!seen);
  }

  // Frame allocator argument: the task frame and every child frame come from it.
  {
    test_context ctx;
    counting_resource mr;
    int got = 0;
    io::run_async(ctx.get_executor(), &mr, [&](int v) { got = v; }, [](std::exception_ptr) {})(posting());
    ctx.run();
    assert(got == 5);
    assert(mr.allocations == 1);
    assert(mr.deallocations == 1);
  }
  // The execution context's own frame allocator is the default.
  {
    test_context ctx;
    counting_resource mr;
    ctx.set_frame_allocator(&mr);
    io::run_async(ctx.get_executor())(posting());
    ctx.run();
    assert(mr.allocations == 1 && mr.deallocations == 1);
  }
  // Stop token and allocator together.
  {
    test_context ctx;
    counting_resource mr;
    std::stop_source src;
    io::run_async(ctx.get_executor(), src.get_token(), &mr)(posting());
    ctx.run();
    assert(mr.allocations == 1 && mr.deallocations == 1);
  }

  // The environment form: no work counting; the allocator and stop token come from the environment.
  {
    test_context ctx;
    counting_resource mr;
    std::stop_source src;
    src.request_stop();
    auto env_executor = ctx.get_executor(); // io_env keeps a reference: it must name an object that outlives it
    io::io_env env{env_executor, src.get_token(), &mr};
    bool seen = false;
    io::run_async(env, [&](bool b) { seen = b; }, [](std::exception_ptr) {})(stop_state());
    ctx.run();
    assert(seen);
    assert(mr.allocations == 1 && mr.deallocations == 1);
    assert(ctx.work_started == 0);
  }

  return 0;
}
