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

// any_read_source, any_write_sink, any_buffer_source and any_buffer_sink round trips, with ready and suspending
// operations of different sizes behind one wrapper.

#include <io_stream>

#include <cassert>
#include <string>

#include "test_stream.h"

namespace io = std::io;
using namespace test_stream;
using test_io::test_context;

// A source of complete reads: short data is an eof error with the partial count.
struct memory_source {
  std::string_view data;
  std::size_t pos = 0;

  template <io::MutableBufferSequence MB>
  ready_op<std::size_t> read(MB b) {
    std::size_t n = io::buffer_copy(b, io::const_buffer(data.data() + pos, data.size() - pos));
    pos += n;
    if (n < io::buffer_size(b))
      return {{io::make_error_code(io::error::eof), n}};
    return {{std::error_code{}, n}};
  }
};

// A suspending operation much larger than the others of the same wrapper: the wrapper's storage must fit it.
struct big_eof_op {
  io::continuation c;
  bool* flag;
  char pad[300] = {};
  bool await_ready() const noexcept { return false; }
  void await_suspend(std::coroutine_handle<> h, const io::io_env* e) {
    c.h = h;
    e->executor.post(c);
  }
  io::io_result<> await_resume() {
    *flag = true;
    return {};
  }
};

struct memory_sink {
  std::string out;
  bool eof = false;

  template <io::ConstBufferSequence CB>
  ready_op<std::size_t> write(CB b) {
    return write(b, false);
  }
  template <io::ConstBufferSequence CB>
  ready_op<std::size_t> write(CB b, bool e) {
    std::size_t old = out.size();
    out.resize(old + io::buffer_size(b));
    std::size_t n = io::buffer_copy(io::mutable_buffer(out.data() + old, out.size() - old), b);
    if (e)
      eof = true;
    return {{std::error_code{}, n}};
  }
  big_eof_op write_eof() { return {{}, &eof}; }
};

static_assert(io::ReadSource<memory_source>);
static_assert(io::WriteSink<memory_sink>);

// A buffer source over a string: two descriptors at most per pull; pull is idempotent until consume.
struct string_buffer_source {
  std::string_view data;
  std::size_t pos = 0;
  int pulls       = 0;

  ready_op<std::size_t> pull(io::const_buffer* arr, std::size_t max) {
    ++pulls;
    std::size_t rest = data.size() - pos;
    if (rest == 0 || max == 0)
      return {{std::error_code{}, 0}};
    if (max == 1 || rest < 2) {
      arr[0] = io::const_buffer(data.data() + pos, rest);
      return {{std::error_code{}, 1}};
    }
    arr[0] = io::const_buffer(data.data() + pos, rest / 2);
    arr[1] = io::const_buffer(data.data() + pos + rest / 2, rest - rest / 2);
    return {{std::error_code{}, 2}};
  }
  void consume(std::size_t n) { pos += n; }
};

struct string_buffer_sink {
  char store[16];
  std::size_t used = 0;
  std::string collected;
  bool eof = false;

  std::size_t prepare(io::mutable_buffer* arr, std::size_t max) {
    if (max == 0)
      return 0;
    arr[0] = io::mutable_buffer(store + used, sizeof store - used);
    return 1;
  }
  ready_op<> flush_and_commit(std::size_t n) {
    used += n;
    collected.append(store, used);
    used = 0;
    return {};
  }
  ready_op<> commit(std::size_t n) { return flush_and_commit(n); }
  ready_op<> commit(std::size_t n, bool e) {
    eof = e;
    return flush_and_commit(n);
  }
  ready_op<> commit_eof() {
    eof = true;
    return {};
  }
};

static_assert(io::BufferSource<string_buffer_source>);
static_assert(io::BufferSink<string_buffer_sink>);

io::task<void> source_to_sink(io::any_read_source& src, io::any_write_sink& dst, std::string& seen, std::error_code* last) {
  char buf[6];
  for (;;) {
    auto [ec, n] = co_await src.read(io::mutable_buffer(buf, sizeof buf));
    seen.append(buf, n);
    if (n != 0) {
      auto [wec, wn] = co_await dst.write(io::const_buffer(buf, n));
      assert(!wec && wn == n);
    }
    if (ec) {
      *last = ec;
      break;
    }
  }
  auto [eec] = co_await dst.write_eof();
  assert(!eec);
}

io::task<void> pump(io::any_buffer_source& src, io::any_buffer_sink& dst, std::size_t* total) {
  for (;;) {
    io::const_buffer in[2];
    auto [ec, count] = co_await src.pull(in, 2);
    assert(!ec);
    if (count == 0)
      break;
    io::mutable_buffer out[2];
    std::size_t room = dst.prepare(out, 2);
    assert(room == 1);
    std::size_t moved = io::buffer_copy(io::mutable_buffer(out[0]), io::const_buffer(in[0]));
    src.consume(moved);
    auto [cec] = co_await dst.commit(moved);
    assert(!cec);
    *total += moved;
  }
  auto [eec] = co_await dst.commit_eof();
  assert(!eec);
}

int main(int, char**) {
  using namespace std::string_view_literals;

  // source -> sink: complete reads of 6 bytes, final short read is eof, write_eof suspends on a big awaitable.
  {
    test_context ctx;
    memory_source src{.data = "0123456789ABCDEFGHIJ"sv};
    memory_sink dst;
    io::any_read_source rs(&src);
    io::any_write_sink ws(&dst);
    std::string seen;
    std::error_code last;
    auto t = source_to_sink(rs, ws, seen, &last);
    test_io::run_root(ctx, t);
    assert(seen == "0123456789ABCDEFGHIJ" && dst.out == seen);
    assert(last == io::cond::eof);
    assert(dst.eof);
    assert(ctx.posted == 2); // start + the write_eof suspension; everything else completed inline
  }

  // buffer source -> buffer sink.
  {
    test_context ctx;
    string_buffer_source src{.data = "callee-owned buffers, zero copy by design"sv};
    string_buffer_sink dst;
    io::any_buffer_source bs(&src);
    io::any_buffer_sink bk(&dst);
    std::size_t total = 0;
    auto t            = pump(bs, bk, &total);
    test_io::run_root(ctx, t);
    assert(total == src.data.size() && dst.collected == src.data && dst.eof);
    assert(ctx.posted == 1);
  }

  // Empty wrappers and moving.
  {
    io::any_stream empty;
    assert(!empty.has_value() && !static_cast<bool>(empty));
    mock_tcp tcp;
    io::any_stream a(&tcp);
    assert(a);
    io::any_stream b(std::move(a));
    assert(b && !a);
    a = std::move(b);
    assert(a && !b);
    io::any_read_stream r(std::move(static_cast<io::any_read_stream&>(a)));
    assert(r);
  }

  return 0;
}
