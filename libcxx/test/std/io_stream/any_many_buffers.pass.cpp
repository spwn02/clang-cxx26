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

// The complete operations of the type-erased wrappers (any_write_sink::write with and without the end-of-stream flag,
// any_read_source::read) really complete for buffer sequences of more than 16 buffers: the wrapped object is given
// successive windows of 16 non-empty buffers (they used to be truncated to the first 16 with a success result). Empty
// buffers do not count, the end of stream goes with the last window, an error or a short read ends the operation with
// the bytes transferred so far, and the windows may be separated by suspensions. any_buffer_sink and any_buffer_source
// have no buffer sequence parameter (the caller supplies the array and its size), so nothing is gathered there.

#include <io_stream>

#include <cassert>
#include <string>
#include <string_view>
#include <vector>

#include "test_stream.h"

namespace io = std::io;
using namespace test_stream;
using test_io::test_context;

// An operation that completes at once, or posts its continuation first.
template <class Res>
struct maybe_suspending_op {
  Res r;
  bool suspend;
  io::continuation c;
  bool await_ready() const noexcept { return !suspend; }
  void await_suspend(std::coroutine_handle<> h, const io::io_env* e) {
    c.h = h;
    e->executor.post(c);
  }
  Res await_resume() { return r; }
};

struct recording_sink {
  std::string out;
  std::vector<std::size_t> windows; // the number of buffers of each write
  std::vector<bool> eof_flags;
  bool suspend         = false;
  int fail_on_window   = -1; // the write with this index fails after consuming half of its bytes
  bool eof             = false;

  maybe_suspending_op<io::io_result<std::size_t>> do_write(std::span<const io::const_buffer> b, bool e) {
    windows.push_back(b.size());
    eof_flags.push_back(e);
    std::size_t total = io::buffer_size(b);
    std::size_t take  = static_cast<int>(windows.size()) - 1 == fail_on_window ? total / 2 : total;
    for (auto const& x : b) {
      std::size_t n = std::min(take, x.size());
      out.append(static_cast<const char*>(x.data()), n);
      take -= n;
    }
    if (e)
      eof = true;
    if (static_cast<int>(windows.size()) - 1 == fail_on_window)
      return {{io::make_error_code(io::error::canceled), total / 2}, suspend, {}};
    return {{std::error_code{}, total}, suspend, {}};
  }
  template <io::ConstBufferSequence CB>
  maybe_suspending_op<io::io_result<std::size_t>> write(CB b) {
    return do_write(b, false);
  }
  template <io::ConstBufferSequence CB>
  maybe_suspending_op<io::io_result<std::size_t>> write(CB b, bool e) {
    return do_write(b, e);
  }
  ready_op<> write_eof() { return {}; }
};

struct recording_source {
  std::string_view data;
  std::size_t pos = 0;
  std::vector<std::size_t> windows;
  bool suspend = false;

  template <io::MutableBufferSequence MB>
  maybe_suspending_op<io::io_result<std::size_t>> read(MB b) {
    std::size_t nbufs = 0;
    for (auto const& x : b)
      nbufs += x.size() != 0;
    windows.push_back(nbufs);
    std::size_t n = io::buffer_copy(b, io::const_buffer(data.data() + pos, data.size() - pos));
    pos += n;
    if (n < io::buffer_size(b))
      return {{io::make_error_code(io::error::eof), n}, suspend, {}};
    return {{std::error_code{}, n}, suspend, {}};
  }
};

static_assert(io::WriteSink<recording_sink>);
static_assert(io::ReadSource<recording_source>);

// `count` buffers of two bytes ("00", "01", ...) over `store`; with `gaps` an empty buffer follows each of them.
static std::vector<io::const_buffer> make_const(const std::string& store, int count, bool gaps) {
  std::vector<io::const_buffer> v;
  for (int i = 0; i != count; ++i) {
    v.emplace_back(store.data() + 2 * i, 2);
    if (gaps)
      v.emplace_back(store.data(), 0);
  }
  return v;
}

static std::string pattern(int bytes) {
  std::string s;
  for (int i = 0; i != bytes; ++i)
    s.push_back(static_cast<char>('a' + i % 26));
  return s;
}

io::task<void> write_body(io::any_write_sink* s, const std::vector<io::const_buffer>* v, bool with_flag, bool flag,
                          std::error_code* ec, std::size_t* n) {
  if (with_flag) {
    auto r = co_await s->write(*v, flag);
    *ec    = std::get<0>(r);
    *n     = std::get<1>(r);
  } else {
    auto r = co_await s->write(*v);
    *ec    = std::get<0>(r);
    *n     = std::get<1>(r);
  }
}

io::task<void> read_body(io::any_read_source* s, const std::vector<io::mutable_buffer>* v, std::error_code* ec,
                         std::size_t* n) {
  auto r = co_await s->read(*v);
  *ec    = std::get<0>(r);
  *n     = std::get<1>(r);
}

static void check_windows(const std::vector<std::size_t>& w, std::size_t total_bufs) {
  std::size_t sum = 0;
  for (std::size_t i = 0; i != w.size(); ++i) {
    assert(w[i] <= 16 && w[i] != 0);
    if (i + 1 != w.size())
      assert(w[i] == 16);
    sum += w[i];
  }
  assert(sum == total_bufs);
}

static void test_write(int count, bool gaps, bool suspend, bool with_flag, bool flag) {
  test_context ctx;
  std::string store = pattern(2 * count);
  auto bufs         = make_const(store, count, gaps);
  recording_sink sink;
  sink.suspend = suspend;
  io::any_write_sink as(&sink);
  std::error_code ec = io::make_error_code(io::error::eof);
  std::size_t n      = 0;
  auto t             = write_body(&as, &bufs, with_flag, flag, &ec, &n);
  test_io::run_root(ctx, t);
  assert(!ec);
  assert(n == store.size());
  assert(sink.out == store);
  check_windows(sink.windows, static_cast<std::size_t>(count));
  assert(sink.windows.size() == static_cast<std::size_t>((count + 15) / 16));
  // The end of the stream goes with the last window only.
  for (std::size_t i = 0; i != sink.eof_flags.size(); ++i)
    assert(sink.eof_flags[i] == (with_flag && flag && i + 1 == sink.eof_flags.size()));
  assert(sink.eof == (with_flag && flag));
}

static void test_write_failure() {
  test_context ctx;
  std::string store = pattern(2 * 40);
  auto bufs         = make_const(store, 40, false);
  recording_sink sink;
  sink.fail_on_window = 1; // the second window consumes half of its 32 bytes and fails
  io::any_write_sink as(&sink);
  std::error_code ec;
  std::size_t n = 0;
  auto t        = write_body(&as, &bufs, false, false, &ec, &n);
  test_io::run_root(ctx, t);
  assert(ec == io::error::canceled);
  assert(n == 32 + 16);
  assert(sink.windows.size() == 2); // nothing is written after the failure
}

static void test_read(int count, bool gaps, bool suspend, std::size_t available) {
  test_context ctx;
  std::string src_data = pattern(static_cast<int>(available));
  std::string dst(2 * static_cast<std::size_t>(count), '\0');
  std::vector<io::mutable_buffer> bufs;
  for (int i = 0; i != count; ++i) {
    bufs.emplace_back(dst.data() + 2 * i, 2);
    if (gaps)
      bufs.emplace_back(dst.data(), 0);
  }
  recording_source src;
  src.data    = src_data;
  src.suspend = suspend;
  io::any_read_source rs(&src);
  std::error_code ec;
  std::size_t n = 0;
  auto t        = read_body(&rs, &bufs, &ec, &n);
  test_io::run_root(ctx, t);
  std::size_t want = dst.size();
  if (available >= want) {
    assert(!ec);
    assert(n == want);
    assert(dst == src_data.substr(0, want));
    check_windows(src.windows, static_cast<std::size_t>(count));
  } else {
    assert(ec == io::cond::eof);
    assert(n == available);
    assert(dst.compare(0, available, src_data) == 0);
  }
}

int main(int, char**) {
  for (bool suspend : {false, true}) {
    for (bool gaps : {false, true}) {
      for (int count : {1, 16, 17, 20, 32, 33, 100}) {
        test_write(count, gaps, suspend, false, false);
        test_write(count, gaps, suspend, true, false);
        test_write(count, gaps, suspend, true, true);
        test_read(count, gaps, suspend, 2 * count);
        test_read(count, gaps, suspend, 2 * count + 7);
      }
      test_read(20, gaps, suspend, 35); // a short read: eof with the 35 bytes of the second window so far
      test_read(100, gaps, suspend, 1);
      test_read(100, gaps, suspend, 0);
    }
  }
  test_write_failure();
  return 0;
}
