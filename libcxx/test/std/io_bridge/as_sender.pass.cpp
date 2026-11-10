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

// P4093R1: as_sender wraps an IoAwaitable as a sender; the receiver's environment answers get_io_executor.

#include <io_bridge>

#include <atomic>
#include <optional>
#include <string>
#include <type_traits>
#include <tuple>
#include <variant>

#include "test_bridge.h"

using namespace test_bridge;

template <class... Ts>
constexpr bool sigs_are(auto sigs) {
  return std::is_same_v<decltype(sigs), ex::completion_signatures<Ts...>>;
}

// A receiver whose environment answers nothing.
struct bare_receiver {
  using receiver_concept = ex::receiver_tag;
  void set_value(auto&&...) && noexcept {}
  void set_error(auto&&) && noexcept {}
  void set_stopped() && noexcept {}
};

template <class S, class R>
concept connectable = requires(S s, R r) { ex::connect(std::move(s), std::move(r)); };

io::task<std::tuple<std::size_t, std::error_code>> value_then_code();
io::task<std::tuple<std::error_code>> one_tuple();

io::task<void> void_task(std::thread::id* id) {
  *id = std::this_thread::get_id();
  co_return;
}
io::task<void> delayed_void(std::thread::id* id) {
  co_await test_io::post_op{};
  *id = std::this_thread::get_id();
}
io::task<int> int_task(int v) {
  co_await test_io::post_op{};
  co_return v;
}
io::task<std::string> string_task() { co_return std::string("hello bridge, long enough to be heap allocated"); }
io::task<std::error_code> ec_task(std::errc e) {
  co_await test_io::post_op{};
  co_return e == std::errc{} ? std::error_code{} : std::make_error_code(e);
}
io::task<io::io_result<>> io_result_void_task(std::errc e) {
  co_await test_io::post_op{};
  co_return io::io_result<>{e == std::errc{} ? std::error_code{} : std::make_error_code(e)};
}
io::task<void> throwing_task() {
  co_await test_io::post_op{};
  TEST_THROW(11);
}

int main(int, char**) {
  io::thread_pool pool(2);
  auto exe = pool.get_executor();
  io::io_sender_env env{io::executor_ref(exe), {}};
  auto run = [&](auto&& sndr) { return std::this_thread::sync_wait(ex::write_env(std::forward<decltype(sndr)>(sndr), env)); };

  // Completion signatures follow the await_resume type.
  static_assert(std::is_same_v<decltype(ex::get_completion_signatures<decltype(io::as_sender(void_task(nullptr))), ex::env<>>()),
                               ex::completion_signatures<ex::set_value_t(), ex::set_error_t(std::exception_ptr), ex::set_stopped_t()>>);
  static_assert(std::is_same_v<decltype(ex::get_completion_signatures<decltype(io::as_sender(int_task(0))), ex::env<>>()),
                               ex::completion_signatures<ex::set_value_t(int), ex::set_error_t(std::exception_ptr), ex::set_stopped_t()>>);
  static_assert(std::is_same_v<decltype(ex::get_completion_signatures<decltype(io::as_sender(ec_task({}))), ex::env<>>()),
                               ex::completion_signatures<ex::set_value_t(), ex::set_error_t(std::error_code),
                                                         ex::set_error_t(std::exception_ptr), ex::set_stopped_t()>>);
  static_assert(std::is_same_v<decltype(ex::get_completion_signatures<decltype(io::as_sender(io_result_void_task({}))), ex::env<>>()),
                               ex::completion_signatures<ex::set_value_t(), ex::set_error_t(std::error_code),
                                                         ex::set_error_t(std::exception_ptr), ex::set_stopped_t()>>);
  static_assert(ex::sender<decltype(io::as_sender(int_task(0)))>);
  // The receiver's environment must answer get_io_executor.
  static_assert(!connectable<decltype(io::as_sender(int_task(0))), bare_receiver>);
  // The floor is structural: only (error_code, more...) is rejected. A tuple that merely contains an error_code, a
  // one-element tuple of error_code and a type without the tuple protocol are accepted.
  static_assert(ex::sender<decltype(io::as_sender(value_then_code()))>);
  static_assert(ex::sender<decltype(io::as_sender(one_tuple()))>);

  // void awaitable: set_value(), and it runs on a pool worker, not on this thread.
  {
    std::thread::id id;
    auto r = run(io::as_sender(delayed_void(&id)));
    assert(r.has_value());
    assert(id != std::thread::id() && id != std::this_thread::get_id());
  }
  {
    std::thread::id id;
    run(io::as_sender(void_task(&id)));
    assert(id != std::this_thread::get_id()); // started by a post to the executor, not inline
  }
  // single value
  {
    auto r = run(io::as_sender(int_task(41)) | ex::then([](int v) { return v + 1; }));
    assert(r && std::get<0>(*r) == 42);
  }
  {
    auto r = run(io::as_sender(string_task()));
    assert(r && std::get<0>(*r).size() > 30);
  }
  // task<error_code>: success -> set_value(), failure -> set_error(ec) reachable by upon_error
  {
    auto r = run(io::as_sender(ec_task(std::errc{})));
    assert(r.has_value());
  }
  {
    std::error_code seen;
    (void)run(io::as_sender(ec_task(std::errc::io_error)) | ex::upon_error([&](auto e) {
                   if constexpr (std::is_same_v<decltype(e), std::error_code>)
                     seen = e;
                 }));
    assert(seen == std::errc::io_error);
  }
  // io_result<>: same mapping
  {
    auto r = run(io::as_sender(io_result_void_task(std::errc{})));
    assert(r.has_value());
  }
  {
    std::error_code seen;
    run(io::as_sender(io_result_void_task(std::errc::timed_out)) | ex::upon_error([&](auto e) {
          if constexpr (std::is_same_v<decltype(e), std::error_code>)
            seen = e;
        }));
    assert(seen == std::errc::timed_out);
  }
  // without upon_error the error surfaces from sync_wait as a system_error
#ifndef TEST_HAS_NO_EXCEPTIONS
  {
    bool caught = false;
    try {
      run(io::as_sender(ec_task(std::errc::io_error)));
    } catch (const std::system_error& e) {
      caught = e.code() == std::errc::io_error;
    }
    assert(caught);
  }
  // exceptions -> set_error(exception_ptr)
  {
    bool caught = false;
    try {
      run(io::as_sender(throwing_task()));
    } catch (int e) {
      caught = e == 11;
    }
    assert(caught);
  }
#endif
  // get_io_executor reaches the operation through adaptors (it is a forwarding query)
  {
    auto sndr = ex::just() | ex::let_value([] { return io::as_sender(int_task(5)); });
    auto r    = run(std::move(sndr));
    assert(r && std::get<0>(*r) == 5);
  }
  // when_all of two bridged awaitables; the inplace_stop_token of when_all is bridged to the awaitable's std::stop_token
  {
    auto r = run(ex::when_all(io::as_sender(int_task(1)), io::as_sender(int_task(2))));
    assert(r && std::get<0>(*r) == 1 && std::get<1>(*r) == 2);
  }
#ifndef TEST_HAS_NO_EXCEPTIONS
  // when_all cancels the sibling when one fails with set_error(ec): the other awaitable sees its stop token triggered
  {
    bool caught = false;
    try {
      run(ex::when_all(io::as_sender(ec_task(std::errc::io_error)), io::as_sender([]() -> io::task<void> {
                         co_await wait_for_stop{};
                       }())));
    } catch (const std::system_error& e) {
      caught = e.code() == std::errc::io_error;
    }
    assert(caught);
  }
#endif
  // the stop token of the receiver's environment becomes the io_env's stop token
  {
    std::stop_source src;
    io::io_sender_env stoppable_env{io::executor_ref(exe), src.get_token()};
    src.request_stop();
    bool stopped = false;
    auto r       = std::this_thread::sync_wait(ex::write_env(io::as_sender(int_task(3)), stoppable_env));
    stopped      = !r.has_value();
    assert(stopped); // the stop was requested before the awaitable completed: set_stopped
  }

  pool.join();
  return 0;
}
