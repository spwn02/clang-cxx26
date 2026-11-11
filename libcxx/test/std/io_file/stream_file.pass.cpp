//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23, c++26
// UNSUPPORTED: no-threads
// REQUIRES: linux || darwin || freebsd
// ADDITIONAL_COMPILE_FLAGS: -fexperimental-library
// <io_file>

// io::stream_file: sequential write, seek, read, the end of the file (error::eof), append, flags, a closed file, the
// Stream concepts and any_stream, a 16 MiB copy through two stream_files from a task, moves.

#include <io_file>

#include <array>
#include <cstring>
#include <string>
#include <type_traits>

#include "test_macros.h"
#include "test_file.h"

using namespace test_file;

static_assert(io::ReadStream<io::stream_file>);
static_assert(io::WriteStream<io::stream_file>);
static_assert(io::Stream<io::stream_file>);
static_assert(std::is_constructible_v<io::any_stream, io::stream_file*>);
static_assert(std::is_nothrow_move_constructible_v<io::stream_file>);
static_assert(std::is_nothrow_move_assignable_v<io::stream_file>);
static_assert(!std::is_copy_constructible_v<io::stream_file>);

io::task<void> write_all(io::any_stream& s, const char* p, std::size_t n) {
  while (n) {
    auto [ec, w] = co_await s.write_some(io::const_buffer(p, n));
    assert(!ec && w > 0);
    p += w;
    n -= w;
  }
}

io::task<std::string> read_to_end(io::any_stream& s) {
  std::string out;
  char buf[1000];
  for (;;) {
    auto [ec, n] = co_await s.read_some(io::mutable_buffer(buf, sizeof buf));
    if (ec == io::error::eof)
      break;
    assert(!ec && n > 0);
    out.append(buf, n);
  }
  co_return out;
}

// Copies from the reader to the writer with a 64 KiB buffer; returns the bytes copied.
io::task<std::size_t> copy(io::stream_file& in, io::stream_file& out) {
  std::string buf(64 * 1024, '\0');
  std::size_t total = 0;
  for (;;) {
    auto [ec, n] = co_await in.read_some(io::mutable_buffer(buf.data(), buf.size()));
    if (ec == io::error::eof)
      break;
    assert(!ec);
    std::size_t done = 0;
    while (done != n) {
      auto [wec, w] = co_await out.write_some(io::const_buffer(buf.data() + done, n - done));
      assert(!wec && w > 0);
      done += w;
    }
    total += n;
  }
  co_return total;
}

int main(int, char**) {
  temp_dir dir;
  io::io_context ctx;

  // write, seek, read, eof, position
  {
    io::stream_file f(ctx);
    assert(!f.open(dir / "a", io::file_base::read_write | io::file_base::create));
    run(ctx, [](io::stream_file& f) -> io::task<void> {
      std::string w = "hello, ";
      auto [e1, n1] = co_await f.write_some(io::const_buffer(w.data(), w.size()));
      assert(!e1 && n1 == 7);
      std::string w2 = "world";
      std::array<io::const_buffer, 2> two{io::const_buffer(w2.data(), 2), io::const_buffer(w2.data() + 2, 3)};
      auto [e2, n2] = co_await f.write_some(two);
      assert(!e2 && n2 == 5);
      auto [es, sz] = f.size();
      assert(!es && sz == 12);
      auto [ep, pos] = f.seek(0);
      assert(!ep && pos == 0);
      char buf[32];
      auto [e3, n3] = co_await f.read_some(io::mutable_buffer(buf, 5));
      assert(!e3 && n3 == 5 && std::memcmp(buf, "hello", 5) == 0);
      auto [ec, pc] = f.seek(2, io::file_base::seek_cur);
      assert(!ec && pc == 7);
      auto [e4, n4] = co_await f.read_some(io::mutable_buffer(buf, 32));
      assert(!e4 && n4 == 5 && std::memcmp(buf, "world", 5) == 0);
      auto [e5, n5] = co_await f.read_some(io::mutable_buffer(buf, 32)); // at the end
      assert(e5 == io::error::eof && n5 == 0);
      auto [ee, pe] = f.seek(-3, io::file_base::seek_end);
      assert(!ee && pe == 9);
      auto [e6, n6] = co_await f.read_some(io::mutable_buffer(buf, 32));
      assert(!e6 && n6 == 3 && std::memcmp(buf, "rld", 3) == 0);
      auto [eb, pb] = f.seek(-1, io::file_base::seek_set);
      assert(eb == std::errc::invalid_argument && pb == 0);
      auto [e7, n7] = co_await f.read_some(io::mutable_buffer(buf, 0)); // nothing to transfer: not the end
      assert(!e7 && n7 == 0);
    }(f));
  }

  // flags and errors
  {
    io::stream_file f(ctx);
    assert(f.open(dir / "none", io::file_base::read_only) == std::errc::no_such_file_or_directory);
    assert(!f.open(dir / "e", io::file_base::write_only | io::file_base::create | io::file_base::exclusive));
    assert(f.open(dir / "e", io::file_base::write_only | io::file_base::create | io::file_base::exclusive) ==
           std::errc::file_exists);
    run(ctx, [](io::stream_file& f, const std::string& p) -> io::task<void> {
      char c[8];
      assert(!f.open(p, io::file_base::write_only));
      auto [e1, n1] = co_await f.read_some(io::mutable_buffer(c, 8));
      assert(e1 == std::errc::bad_file_descriptor && n1 == 0);
      assert(!f.open(p, io::file_base::read_only));
      auto [e2, n2] = co_await f.write_some(io::const_buffer(c, 8));
      assert(e2 == std::errc::bad_file_descriptor && n2 == 0);
      // append: every write goes to the end
      assert(!f.open(p, io::file_base::write_only | io::file_base::append));
      std::string a = "AAA", b = "BB";
      (void)co_await f.write_some(io::const_buffer(a.data(), a.size()));
      auto [ep, pp] = f.seek(0);
      assert(!ep);
      (void)co_await f.write_some(io::const_buffer(b.data(), b.size()));
      assert(std::get<1>(f.size()) == 5);
      // truncate
      assert(!f.open(p, io::file_base::read_write | io::file_base::truncate));
      assert(std::get<1>(f.size()) == 0);
    }(f, dir / "e"));
  }

  // through any_stream, and a closed file
  {
    io::stream_file f(ctx);
    assert(!f.open(dir / "g", io::file_base::read_write | io::file_base::create));
    run(ctx, [](io::stream_file& f) -> io::task<void> {
      io::any_stream s(&f);
      std::string msg = "through any_stream";
      co_await write_all(s, msg.data(), msg.size());
      (void)f.seek(0);
      std::string got = co_await read_to_end(s);
      assert(got == msg);
      f.close();
      char c[4];
      auto [e1, n1] = co_await s.read_some(io::mutable_buffer(c, 4)); // the stream sees a closed file
      assert(e1 == std::errc::bad_file_descriptor && n1 == 0);
      auto [e2, n2] = co_await f.write_some(io::const_buffer(c, 4));
      assert(e2 == std::errc::bad_file_descriptor && n2 == 0);
      auto [e3, p3] = f.seek(0);
      assert(e3 == std::errc::bad_file_descriptor);
    }(f));
  }

  // 16 MiB through two stream_files
  {
    constexpr std::size_t N = 16u << 20;
    {
      std::string data(N, '\0');
      unsigned x = 12345;
      for (std::size_t i = 0; i != N; ++i) {
        x       = x * 1664525u + 1013904223u;
        data[i] = static_cast<char>(x >> 24);
      }
      io::stream_file w(ctx);
      assert(!w.open(dir / "big_in", io::file_base::write_only | io::file_base::create | io::file_base::truncate));
      run(ctx, [](io::stream_file& w, const std::string& data) -> io::task<void> {
        io::any_stream s(&w);
        co_await write_all(s, data.data(), data.size());
      }(w, data));
      w.close();
      io::stream_file in(ctx), out(ctx);
      assert(!in.open(dir / "big_in", io::file_base::read_only));
      assert(!out.open(dir / "big_out", io::file_base::write_only | io::file_base::create));
      std::size_t copied = 0;
      run(
          ctx,
          [](io::stream_file& in, io::stream_file& out, std::size_t* copied) -> io::task<void> {
            *copied = co_await copy(in, out);
          }(in, out, &copied),
          ms(20000));
      assert(copied == N);
      out.close();
      io::stream_file chk(ctx);
      assert(!chk.open(dir / "big_out", io::file_base::read_only));
      assert(std::get<1>(chk.size()) == N);
      std::string back;
      run(
          ctx,
          [](io::stream_file& chk, std::string* back) -> io::task<void> {
            io::any_stream s(&chk);
            *back = co_await read_to_end(s);
          }(chk, &back),
          ms(20000));
      assert(back == data);
    }
  }

  // moves; the position goes with the descriptor
  {
    io::stream_file f(ctx);
    assert(!f.open(dir / "m", io::file_base::read_write | io::file_base::create));
    io::stream_file g(std::move(f));
    assert(!f.is_open() && g.is_open());
    io::stream_file h(ctx.get_executor());
    h = std::move(g);
    assert(h.is_open() && !g.is_open());
    auto [er, fd] = h.release();
    assert(!er && fd >= 0);
    assert(!g.assign(fd));
    assert(g.is_open() && g.native_handle() == fd);
  }
  return 0;
}
