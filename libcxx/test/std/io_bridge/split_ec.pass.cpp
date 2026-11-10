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

// P4093R1 section 8: split_ec turns set_value(error_code) into set_value() or set_error(ec) so that upon_error is
// reachable. The signatures are derived from the child's.

#include <io_bridge>

#include <type_traits>

#include "test_bridge.h"

using namespace test_bridge;

int main(int, char**) {
  using value_ec = decltype(ex::just(std::error_code{}));
  using split_t  = decltype(ex::just(std::error_code{}) | io::split_ec());
  static_assert(std::is_same_v<decltype(ex::get_completion_signatures<split_t, ex::env<>>()),
                               ex::completion_signatures<ex::set_value_t(), ex::set_error_t(std::error_code)>>);
  // other completions of the child pass through
  using split_stopped = decltype(ec_sender{} | ex::then([](int) { return std::error_code{}; }) | io::split_ec());
  static_assert(std::is_same_v<decltype(ex::get_completion_signatures<split_stopped, ex::env<>>()),
                               ex::completion_signatures<ex::set_value_t(), ex::set_error_t(std::error_code),
                                                         ex::set_error_t(std::exception_ptr), ex::set_stopped_t()>>);
  static_assert(std::is_same_v<decltype(io::split_ec(ex::just(std::error_code{}))), split_t>);

  // success: set_value(), upon_error not called
  {
    bool error_seen = false;
    auto r = std::this_thread::sync_wait(ex::just(std::error_code{}) | io::split_ec() |
                                         ex::upon_error([&](std::error_code) { error_seen = true; }));
    assert(r.has_value() && !error_seen);
  }
  // failure: set_error(ec), upon_error reached
  {
    std::error_code seen;
    auto r = std::this_thread::sync_wait(ex::just(std::make_error_code(std::errc::io_error)) | io::split_ec() |
                                         ex::upon_error([&](std::error_code ec) { seen = ec; }));
    assert(r.has_value());
    assert(seen == std::errc::io_error);
  }
  // a plain (void) value completion is accepted
  {
    auto r = std::this_thread::sync_wait(ex::just() | io::split_ec() | ex::then([] { return 3; }));
    assert(r && std::get<0>(*r) == 3);
  }
  // the pipe form and the call form are the same
  {
    std::error_code seen;
    std::this_thread::sync_wait(io::split_ec(ex::just(std::make_error_code(std::errc::timed_out))) |
                                ex::upon_error([&](std::error_code ec) { seen = ec; }));
    assert(seen == std::errc::timed_out);
  }
  // a closure composes with other closures
  {
    std::error_code seen;
    auto closure = io::split_ec() | ex::upon_error([&](std::error_code ec) { seen = ec; });
    std::this_thread::sync_wait(ex::just(std::make_error_code(std::errc::io_error)) | closure);
    assert(seen == std::errc::io_error);
  }
  // an unhandled failure reaches sync_wait as an exception
#ifndef TEST_HAS_NO_EXCEPTIONS
  {
    bool caught = false;
    try {
      std::this_thread::sync_wait(ex::just(std::make_error_code(std::errc::io_error)) | io::split_ec());
    } catch (const std::system_error& e) {
      caught = e.code() == std::errc::io_error;
    }
    assert(caught);
  }
#endif
  // no allocation, no type erasure: the adapted operation is as large as a bare operation plus the receiver
  static_assert(sizeof(split_t) == sizeof(value_ec));
  return 0;
}
