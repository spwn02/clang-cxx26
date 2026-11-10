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

// <io_signal>

// std::io::signal_set: add/remove/clear/wait/cancel, signals that arrive before the wait are remembered, restoring of
// the previous disposition, flag conflicts between sets, cancellation by a stop token, several sets on one signal.
// Only SIGUSR1 and SIGUSR2 are used.

#include <io_signal>
#include <io_context>
#include <io_timer>

#include <cassert>
#include <csignal>
#include <stop_token>
#include <thread>
#include <unistd.h>

#include "test_ctx.h"
#include "test_macros.h"

using namespace test_ctx;

static volatile std::sig_atomic_t old_handler_calls = 0;
extern "C" void old_handler(int) { old_handler_calls = old_handler_calls + 1; }

static bool is_canceled(std::error_code ec) { return ec == io::cond::canceled; }

io::task<void> wait_sig(io::signal_set& s, std::error_code& ec, int& signo, std::thread::id* where = nullptr) {
  auto [e, n] = co_await s.wait();
  ec          = e;
  signo       = n;
  if (where)
    *where = std::this_thread::get_id();
}

static void send_after(int sig, ms d) {
  std::thread([sig, d] {
    std::this_thread::sleep_for(d);
    ::kill(::getpid(), sig);
  }).detach();
}

int main(int, char**) {
  using flags = io::signal_set;

  // A wait completes with the number of the signal that is raised later; the coroutine resumes on the context's thread.
  {
    io::io_context ctx;
    io::signal_set s(ctx);
    assert(!s.add(SIGUSR1));
    assert(!s.add(SIGUSR2));
    std::error_code ec = std::make_error_code(std::errc::invalid_argument);
    int signo          = 0;
    std::thread::id where;
    io::run_async(ctx.get_executor())(wait_sig(s, ec, signo, &where));
    send_after(SIGUSR2, ms(30));
    watchdog wd(ctx);
    ctx.run();
    assert(!ec);
    assert(signo == SIGUSR2);
    assert(where == std::this_thread::get_id());
  }

  // A signal that arrives while nobody waits is remembered, one per arrival.
  {
    io::io_context ctx;
    io::signal_set s(ctx, SIGUSR1);
    std::raise(SIGUSR1);
    std::raise(SIGUSR1);
    ctx.poll(); // drains the pipe (nothing waits yet)
    for (int i = 0; i != 2; ++i) {
      std::error_code ec;
      int signo = 0;
      ctx.restart();
      io::run_async(ctx.get_executor())(wait_sig(s, ec, signo));
      watchdog wd(ctx);
      ctx.run();
      assert(!ec && signo == SIGUSR1);
    }
  }

  // cancel() completes the wait with canceled; the signal stays registered.
  {
    io::io_context ctx;
    io::signal_set s(ctx, SIGUSR1);
    std::error_code ec;
    int signo = -1;
    io::run_async(ctx.get_executor())(wait_sig(s, ec, signo));
    ctx.poll();
    s.cancel();
    watchdog wd(ctx);
    ctx.restart();
    ctx.run();
    assert(is_canceled(ec) && signo == 0);
    std::error_code ec2;
    int signo2 = 0;
    ctx.restart();
    io::run_async(ctx.get_executor())(wait_sig(s, ec2, signo2));
    send_after(SIGUSR1, ms(20));
    ctx.run();
    assert(!ec2 && signo2 == SIGUSR1);
  }

  // Stop token cancellation.
  {
    io::io_context ctx;
    io::signal_set s(ctx, SIGUSR2);
    std::stop_source src;
    std::error_code ec;
    int signo = 0;
    io::run_async(ctx.get_executor(), src.get_token())(wait_sig(s, ec, signo));
    std::thread t([&] {
      std::this_thread::sleep_for(ms(30));
      src.request_stop();
    });
    watchdog wd(ctx);
    ctx.run();
    t.join();
    assert(is_canceled(ec));
  }

  // The previous disposition comes back when the last set with the signal is destroyed, removed or cleared.
  {
    struct sigaction mine {};
    mine.sa_handler = old_handler;
    sigemptyset(&mine.sa_mask);
    struct sigaction before;
    assert(::sigaction(SIGUSR1, &mine, &before) == 0);
    auto current = [] {
      struct sigaction cur;
      ::sigaction(SIGUSR1, nullptr, &cur);
      return cur.sa_handler;
    };
    assert(current() == old_handler);
    {
      io::io_context ctx;
      {
        io::signal_set a(ctx);
        io::signal_set b(ctx);
        assert(!a.add(SIGUSR1));
        assert(current() != old_handler);
        assert(!b.add(SIGUSR1));
        assert(!a.remove(SIGUSR1));
        assert(current() != old_handler); // b still has it
        assert(!a.remove(SIGUSR1));       // not in the set: no effect
        assert(!b.clear());
        assert(current() == old_handler);
        assert(!b.add(SIGUSR1));
        assert(current() != old_handler);
      } // b destroyed
      assert(current() == old_handler);
      // A set in a second context counts too.
      io::io_context ctx2;
      {
        io::signal_set a(ctx), b(ctx2);
        assert(!a.add(SIGUSR1));
        assert(!b.add(SIGUSR1));
      }
      assert(current() == old_handler);
    }
    // With the old handler in place raising still calls it (nobody has the signal).
    old_handler_calls = 0;
    std::raise(SIGUSR1);
    assert(old_handler_calls == 1);
    ::sigaction(SIGUSR1, &before, nullptr);
  }

  // Invalid signals, flags and conflicts between sets.
  {
    io::io_context ctx;
    io::signal_set a(ctx), b(ctx);
    assert(a.add(0) == std::errc::invalid_argument);
    assert(a.add(1000) == std::errc::invalid_argument);
    assert(a.add(SIGKILL));  // cannot be caught
    assert(a.add(SIGUSR1, flags::flags_t(1u << 9)) == std::errc::invalid_argument);
    assert(!a.add(SIGUSR1, flags::restart));
    assert(!a.add(SIGUSR1, flags::restart));                   // same flags: no effect
    assert(b.add(SIGUSR1, flags::none) == std::errc::invalid_argument); // different flags
    assert(!b.add(SIGUSR1, flags::dont_care));                  // compatible with anything
    assert(!b.add(SIGUSR1, flags::restart));                    // the same as the installed ones
    struct sigaction cur;
    ::sigaction(SIGUSR1, nullptr, &cur);
    assert(cur.sa_flags & SA_RESTART);
    assert(!a.clear());
    assert(!b.clear());
    // Combining flags.
    assert(!a.add(SIGUSR2, flags::restart | flags::no_defer));
    ::sigaction(SIGUSR2, nullptr, &cur);
    assert((cur.sa_flags & SA_RESTART) && (cur.sa_flags & SA_NODEFER));
    // dont_care first: a later concrete flag decides and is applied.
    assert(!b.add(SIGUSR1, flags::dont_care));
    assert(!a.add(SIGUSR1, flags::restart));
    ::sigaction(SIGUSR1, nullptr, &cur);
    assert(cur.sa_flags & SA_RESTART);
  }

  // Two sets (two contexts) waiting for the same signal both get it.
  {
    io::io_context c1, c2;
    io::signal_set s1(c1, SIGUSR1), s2(c2, SIGUSR1);
    std::error_code e1, e2;
    int n1 = 0, n2 = 0;
    io::run_async(c1.get_executor())(wait_sig(s1, e1, n1));
    io::run_async(c2.get_executor())(wait_sig(s2, e2, n2));
    send_after(SIGUSR1, ms(30));
    watchdog w1(c1), w2(c2);
    std::thread t([&] { c2.run(); });
    c1.run();
    t.join();
    assert(!e1 && n1 == SIGUSR1);
    assert(!e2 && n2 == SIGUSR1);
  }

  // A second concurrent wait on one set is refused.
  {
    io::io_context ctx;
    io::signal_set s(ctx, SIGUSR1);
    std::error_code e1, e2;
    int n1 = 0, n2 = 0;
    io::run_async(ctx.get_executor())(wait_sig(s, e1, n1));
    io::run_async(ctx.get_executor())(wait_sig(s, e2, n2));
    ctx.poll();
    assert(e2 == std::errc::operation_in_progress);
    s.cancel();
    watchdog wd(ctx);
    ctx.restart();
    ctx.run();
    assert(is_canceled(e1));
  }

  // The constructor with signals throws on failure (not testable with -fno-exceptions).
#ifndef TEST_HAS_NO_EXCEPTIONS
  {
    io::io_context ctx;
    bool threw = false;
    try {
      io::signal_set s(ctx, SIGUSR1, SIGKILL);
    } catch (const std::system_error&) {
      threw = true;
    }
    assert(threw);
  }
#endif

  return 0;
}
