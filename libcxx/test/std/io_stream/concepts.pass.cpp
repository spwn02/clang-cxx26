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

// <io_stream>

// The stream concepts: ReadStream, WriteStream, Stream, ReadSource, WriteSink, BufferSource, BufferSink.

#include <io_stream>

#include <coroutine>
#include <cstddef>
#include <system_error>

#include "test_stream.h"

namespace io = std::io;
using namespace test_stream;

// Satisfy only the syntactic requirement of a ready awaitable of (error_code, size_t).
struct plain_awaitable { // not an IoAwaitable: await_suspend takes no environment
  bool await_ready() const noexcept { return true; }
  void await_suspend(std::coroutine_handle<>) const noexcept {}
  std::tuple<std::error_code, std::size_t> await_resume() const { return {}; }
};
struct not_io_read {
  template <io::MutableBufferSequence MB>
  plain_awaitable read_some(MB) { return {}; }
};

struct wrong_result_op : ready_op<> {}; // result is (error_code) only, not (error_code, size_t)
struct wrong_result_read {
  template <io::MutableBufferSequence MB>
  wrong_result_op read_some(MB) { return {}; }
};

struct returns_int {
  int read_some(io::mutable_buffer) { return 0; }
};

struct only_mutable_buffer { // read_some accepts one concrete type: not a MutableBufferSequence template
  ready_op<std::size_t> read_some(io::mutable_buffer) { return {}; }
};

struct const_only_read { // read_some takes ConstBufferSequence: wrong direction
  template <io::ConstBufferSequence CB>
  ready_op<std::size_t> read_some(CB) { return {}; }
};

struct source_and_sink {
  template <io::MutableBufferSequence MB>
  ready_op<std::size_t> read(MB) { return {}; }
  template <io::ConstBufferSequence CB>
  ready_op<std::size_t> write(CB) { return {}; }
  template <io::ConstBufferSequence CB>
  ready_op<std::size_t> write(CB, bool) { return {}; }
  ready_op<> write_eof() { return {}; }
};

struct buf_source {
  ready_op<std::size_t> pull(io::const_buffer*, std::size_t) { return {}; }
  void consume(std::size_t) {}
};
struct buf_sink {
  std::size_t prepare(io::mutable_buffer*, std::size_t) { return 0; }
  ready_op<> commit(std::size_t) { return {}; }
  ready_op<> commit(std::size_t, bool) { return {}; }
  ready_op<> commit_eof() { return {}; }
};

// ReadStream / WriteStream / Stream
static_assert(io::ReadStream<memory_read_stream>);
static_assert(!io::WriteStream<memory_read_stream>);
static_assert(io::WriteStream<memory_write_stream>);
static_assert(!io::ReadStream<memory_write_stream>);
static_assert(io::ReadStream<deferred_read_stream>);
static_assert(io::WriteStream<deferred_write_stream>);
static_assert(io::Stream<mock_tcp>);
static_assert(!io::Stream<memory_read_stream>);
static_assert(!io::Stream<deferred_read_stream>);

// Negative cases for ReadStream.
static_assert(!io::ReadStream<int>);
static_assert(!io::ReadStream<not_io_read>);      // read_some returns a non-IoAwaitable
static_assert(!io::ReadStream<wrong_result_read>); // result does not decompose to (error_code, size_t)
static_assert(!io::ReadStream<returns_int>);
// The archetype is a mutable buffer sequence (and so also a const one, like every real mutable sequence): a read_some
// that takes one concrete buffer type, or one that is only constrained on the const concept, satisfies the syntactic
// check. Only the operations that cannot accept the archetype at all are rejected.
static_assert(io::ReadStream<only_mutable_buffer>);
static_assert(io::ReadStream<const_only_read>);
static_assert(!io::IoAwaitable<plain_awaitable>);

// Sources and sinks.
static_assert(io::ReadSource<source_and_sink>);
static_assert(io::WriteSink<source_and_sink>);
static_assert(!io::ReadSource<memory_read_stream>);
static_assert(!io::WriteSink<memory_write_stream>);
static_assert(!io::WriteSink<buf_sink>);
static_assert(io::BufferSource<buf_source>);
static_assert(!io::BufferSource<buf_sink>);
static_assert(io::BufferSink<buf_sink>);
static_assert(!io::BufferSink<buf_source>);
static_assert(!io::BufferSource<memory_read_stream>);

// The type-erasing wrappers satisfy the concepts they erase (so wrappers compose with the generic algorithms).
static_assert(io::ReadStream<io::any_read_stream> && !io::WriteStream<io::any_read_stream>);
static_assert(io::WriteStream<io::any_write_stream> && !io::ReadStream<io::any_write_stream>);
static_assert(io::Stream<io::any_stream>);
static_assert(io::ReadStream<io::any_stream> && io::WriteStream<io::any_stream>);
static_assert(io::ReadSource<io::any_read_source> && !io::WriteSink<io::any_read_source>);
static_assert(io::WriteSink<io::any_write_sink>);
static_assert(io::BufferSource<io::any_buffer_source>);
static_assert(io::BufferSink<io::any_buffer_sink>);

// Wrappers are not copyable; they are movable.
static_assert(!std::is_copy_constructible_v<io::any_stream>);
static_assert(std::is_nothrow_move_constructible_v<io::any_stream>);
static_assert(std::is_nothrow_default_constructible_v<io::any_read_stream>);

// A wrapper is built from a pointer to a conforming object only.
static_assert(std::is_constructible_v<io::any_read_stream, memory_read_stream*>);
static_assert(!std::is_constructible_v<io::any_read_stream, memory_write_stream*>);
static_assert(!std::is_constructible_v<io::any_read_stream, memory_read_stream>); // no owning mode
static_assert(!std::is_constructible_v<io::any_stream, memory_read_stream*>);     // needs both directions
static_assert(std::is_constructible_v<io::any_stream, mock_tcp*>);
static_assert(std::is_constructible_v<io::any_write_sink, source_and_sink*>);
static_assert(std::is_constructible_v<io::any_buffer_sink, buf_sink*>);

int main(int, char**) { return 0; }
