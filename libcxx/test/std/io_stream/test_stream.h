//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef TEST_STD_IO_STREAM_TEST_STREAM_H
#define TEST_STD_IO_STREAM_TEST_STREAM_H

#include <algorithm>
#include <cassert>
#include <coroutine>
#include <cstddef>
#include <io>
#include <io_buffer>
#include <io_stream>
#include <string>
#include <string_view>
#include <system_error>

#include "../io/test_io.h"
#include "test_macros.h"

namespace test_stream {

namespace io = std::io;

// An already-completed operation: await_ready is true, so a coroutine never suspends on it.
template <class... Ts>
struct ready_op {
  io::io_result<Ts...> r;
  bool await_ready() const noexcept { return true; }
  void await_suspend(std::coroutine_handle<>, const io::io_env*) const noexcept {}
  io::io_result<Ts...> await_resume() { return r; }
};

// A read stream over a string. Every read_some delivers at most `chunk` bytes, and completes immediately.
// After `fail_after` bytes were delivered the stream reports `fail_ec` (when set); at the end it reports eof.
struct memory_read_stream {
  std::string_view data;
  std::size_t pos     = 0;
  std::size_t chunk   = std::size_t(-1);
  std::size_t fail_at = std::size_t(-1);
  std::error_code fail_ec{};
  int calls = 0;

  template <io::MutableBufferSequence MB>
  ready_op<std::size_t> read_some(MB b) {
    ++calls;
    if (io::buffer_empty(b))
      return {{std::error_code{}, 0}};
    if (pos >= fail_at)
      return {{fail_ec, 0}};
    std::size_t avail = data.size() - pos;
    if (avail == 0)
      return {{io::make_error_code(io::error::eof), 0}};
    std::size_t lim = std::min(avail, chunk);
    if (fail_at != std::size_t(-1))
      lim = std::min(lim, fail_at - pos);
    std::size_t n = io::buffer_copy(b, io::const_buffer(data.data() + pos, lim));
    pos += n;
    return {{std::error_code{}, n}};
  }
};

// A write stream appending to a string; accepts at most `chunk` bytes per call.
struct memory_write_stream {
  std::string out;
  std::size_t chunk   = std::size_t(-1);
  std::size_t fail_at = std::size_t(-1); // bytes after which it reports fail_ec
  std::error_code fail_ec{};
  int calls = 0;

  template <io::ConstBufferSequence CB>
  ready_op<std::size_t> write_some(CB b) {
    ++calls;
    if (io::buffer_empty(b))
      return {{std::error_code{}, 0}};
    if (out.size() >= fail_at)
      return {{fail_ec, 0}};
    std::size_t lim = std::min(io::buffer_size(b), chunk);
    if (fail_at != std::size_t(-1))
      lim = std::min(lim, fail_at - out.size());
    std::size_t old = out.size();
    out.resize(old + lim);
    std::size_t n = io::buffer_copy(io::mutable_buffer(out.data() + old, lim), b);
    return {{std::error_code{}, n}};
  }
};

// A read stream that always suspends: it posts the continuation to the executor, and does its work when resumed.
// If the stop token is stopped at that point it reports canceled and delivers nothing.
template <class MB>
struct deferred_read_op {
  memory_read_stream* s;
  MB b;
  io::continuation c;
  const io::io_env* env = nullptr;

  bool await_ready() const noexcept { return false; }
  void await_suspend(std::coroutine_handle<> h, const io::io_env* e) {
    env = e;
    c.h = h;
    e->executor.post(c);
  }
  io::io_result<std::size_t> await_resume() {
    if (env->stop_token.stop_requested())
      return {io::make_error_code(io::error::canceled), 0};
    return s->read_some(b).await_resume();
  }
};

struct deferred_read_stream {
  memory_read_stream inner;
  int suspensions = 0;

  template <io::MutableBufferSequence MB>
  deferred_read_op<MB> read_some(MB b) {
    ++suspensions;
    return {&inner, b, {}, nullptr};
  }
};

template <class CB>
struct deferred_write_op {
  memory_write_stream* s;
  CB b;
  io::continuation c;
  const io::io_env* env = nullptr;

  bool await_ready() const noexcept { return false; }
  void await_suspend(std::coroutine_handle<> h, const io::io_env* e) {
    env = e;
    c.h = h;
    e->executor.post(c);
  }
  io::io_result<std::size_t> await_resume() {
    if (env->stop_token.stop_requested())
      return {io::make_error_code(io::error::canceled), 0};
    return s->write_some(b).await_resume();
  }
};

struct deferred_write_stream {
  memory_write_stream inner;
  int suspensions = 0;

  template <io::ConstBufferSequence CB>
  deferred_write_op<CB> write_some(CB b) {
    ++suspensions;
    return {&inner, b, {}, nullptr};
  }
};

// A tcp-like duplex stream: reads come from `in` and alternate between suspending and completing inline, writes go to
// `out` and always suspend.
struct mock_tcp {
  memory_read_stream rd;
  memory_write_stream wr;
  int toggle = 0;

  template <io::MutableBufferSequence MB>
  auto read_some(MB b) {
    struct op {
      mock_tcp* t;
      MB b;
      bool suspend;
      io::continuation c;
      bool await_ready() const noexcept { return !suspend; }
      void await_suspend(std::coroutine_handle<> h, const io::io_env* e) {
        c.h = h;
        e->executor.post(c);
      }
      io::io_result<std::size_t> await_resume() { return t->rd.read_some(b).await_resume(); }
    };
    return op{this, b, (toggle++ % 2) == 0, {}};
  }

  template <io::ConstBufferSequence CB>
  deferred_write_op<CB> write_some(CB b) {
    return {&wr, b, {}, nullptr};
  }
};

} // namespace test_stream

#endif // TEST_STD_IO_STREAM_TEST_STREAM_H
