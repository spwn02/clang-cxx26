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

// <io_file>

// io::random_access_file: write and read back at offsets (out of order, past the end, gaps), scatter/gather buffers,
// size/resize/sync, the open flags and their errors, operations on a closed file, moves, release/assign, std::filesystem::path.

#include <io_file>

#include <array>
#include <cstring>
#include <fcntl.h>
#include <string>
#include <type_traits>
#include <vector>

#include "test_macros.h"
#include "test_file.h"

using namespace test_file;

static_assert(!io::ReadStream<io::random_access_file>);
static_assert(!io::WriteStream<io::random_access_file>);
static_assert(std::is_nothrow_move_constructible_v<io::random_access_file>);
static_assert(std::is_nothrow_move_assignable_v<io::random_access_file>);
static_assert(!std::is_copy_constructible_v<io::random_access_file>);
static_assert(std::is_same_v<io::open_flags, io::file_base::flags>);
static_assert((io::random_access_file::read_only | io::random_access_file::write_only) == io::file_base::read_write);

io::task<std::string> read_at(io::random_access_file& f, std::uint64_t off, std::size_t n, std::error_code* ecp) {
  std::string s(n, '\0');
  auto [ec, got] = co_await f.read_some_at(off, io::mutable_buffer(s.data(), s.size()));
  if (ecp)
    *ecp = ec;
  s.resize(got);
  co_return s;
}

int main(int, char**) {
  temp_dir dir;
  io::io_context ctx;

  // write, read back, out of order, gaps, past the end
  {
    io::random_access_file f(ctx);
    assert(!f.is_open() && f.native_handle() == -1);
    assert(!f.open(dir / "a", io::file_base::read_write | io::file_base::create));
    assert(f.is_open() && f.native_handle() >= 0);
    std::string out;
    run(ctx, [](io::random_access_file& f, std::string& out) -> io::task<void> {
      std::string w1 = "hello";
      auto [e1, n1]  = co_await f.write_some_at(10, io::const_buffer(w1.data(), w1.size()));
      assert(!e1 && n1 == 5);
      std::string w0 = "01234";
      auto [e0, n0]  = co_await f.write_some_at(0, io::const_buffer(w0.data(), w0.size()));
      assert(!e0 && n0 == 5);
      auto [es, sz] = f.size();
      assert(!es && sz == 15);
      std::error_code ec;
      std::string r = co_await read_at(f, 8, 20, &ec); // 2 zero bytes of the gap, then "hello", then the end
      assert(!ec && r.size() == 7 && r[0] == 0 && r[1] == 0 && r.substr(2) == "hello");
      r = co_await read_at(f, 0, 5, &ec);
      assert(!ec && r == "01234");
      r = co_await read_at(f, 15, 4, &ec); // at the end
      assert(ec == io::error::eof && r.empty());
      r = co_await read_at(f, 1000, 4, &ec); // beyond the end
      assert(ec == io::error::eof && r.empty());
      // an empty buffer transfers nothing and is not the end of the file
      auto [ee, ne] = co_await f.read_some_at(1000, io::mutable_buffer(nullptr, 0));
      assert(!ee && ne == 0);
      out = "ok";
    }(f, out));
    assert(out == "ok");
  }

  // scatter / gather
  {
    io::random_access_file f(ctx);
    assert(!f.open(dir / "b", io::file_base::read_write | io::file_base::create | io::file_base::truncate));
    run(ctx, [](io::random_access_file& f) -> io::task<void> {
      std::string a = "abc", b = "", c = "defgh", d = "ij";
      std::array<io::const_buffer, 4> bufs{
          io::const_buffer(a.data(), a.size()),
          io::const_buffer(b.data(), 0),
          io::const_buffer(c.data(), c.size()),
          io::const_buffer(d.data(), d.size())};
      auto [ew, nw] = co_await f.write_some_at(3, bufs);
      assert(!ew && nw == 10);
      char x[4] = {}, y[8] = {}, z[8] = {};
      std::array<io::mutable_buffer, 3> rb{
          io::mutable_buffer(x, 4), io::mutable_buffer(y, 0), io::mutable_buffer(z, 8)};
      auto [er, nr] = co_await f.read_some_at(3, rb);
      assert(!er && nr == 10);
      assert(std::memcmp(x, "abcd", 4) == 0 && std::memcmp(z, "efghij", 6) == 0);
      // a vector of buffers
      std::vector<io::mutable_buffer> v{io::mutable_buffer(x, 2), io::mutable_buffer(y, 2)};
      auto [ev, nv] = co_await f.read_some_at(5, v);
      assert(!ev && nv == 4 && std::memcmp(x, "cd", 2) == 0 && std::memcmp(y, "ef", 2) == 0);
    }(f));
  }

  // size / resize / sync
  {
    io::random_access_file f(ctx);
    assert(!f.open(dir / "c", io::file_base::read_write | io::file_base::create));
    assert(!f.resize(4096));
    auto [e1, s1] = f.size();
    assert(!e1 && s1 == 4096);
    assert(!f.resize(10));
    auto [e2, s2] = f.size();
    assert(!e2 && s2 == 10);
    assert(!f.sync_data());
    assert(!f.sync_all());
  }

  // flags: create / exclusive / truncate / append / read_only
  {
    io::random_access_file f(ctx);
    auto ec = f.open(dir / "missing", io::file_base::read_only);
    assert(ec == std::errc::no_such_file_or_directory && !f.is_open());
    ec = f.open(dir / "missing", io::file_base::read_write);
    assert(ec == std::errc::no_such_file_or_directory);
    assert(!f.open(dir / "d", io::file_base::write_only | io::file_base::create | io::file_base::exclusive));
    ec = f.open(dir / "d", io::file_base::write_only | io::file_base::create | io::file_base::exclusive);
    assert(ec == std::errc::file_exists && !f.is_open()); // the file that was open is closed first
    ec = f.open(dir / "no_such_dir/x", io::file_base::write_only | io::file_base::create);
    assert(ec == std::errc::no_such_file_or_directory);
    ec = f.open(std::string_view("a\0b", 3), io::file_base::read_only);
    assert(ec == std::errc::invalid_argument);

    run(ctx, [](io::random_access_file& f, const std::string& path) -> io::task<void> {
      // write_only: reading fails with EBADF; the file was created above and is empty
      assert(!f.open(path, io::file_base::write_only));
      std::string w = "0123456789";
      auto [e1, n1] = co_await f.write_some_at(0, io::const_buffer(w.data(), w.size()));
      assert(!e1 && n1 == 10);
      char c[4];
      auto [e2, n2] = co_await f.read_some_at(0, io::mutable_buffer(c, 4));
      assert(e2 == std::errc::bad_file_descriptor && n2 == 0);
      // read_only: writing fails with EBADF
      assert(!f.open(path, io::file_base::read_only));
      auto [e3, n3] = co_await f.write_some_at(0, io::const_buffer(w.data(), w.size()));
      assert(e3 == std::errc::bad_file_descriptor && n3 == 0);
      auto [e4, n4] = co_await f.read_some_at(2, io::mutable_buffer(c, 4));
      assert(!e4 && n4 == 4 && std::memcmp(c, "2345", 4) == 0);
      // append: writes go to the end whatever the offset
      assert(!f.open(path, io::file_base::read_write | io::file_base::append));
      std::string ap = "AB";
      auto [e5, n5]  = co_await f.write_some_at(0, io::const_buffer(ap.data(), ap.size()));
      assert(!e5 && n5 == 2);
      auto [es, sz] = f.size();
      assert(!es && sz == 12);
      // truncate empties the file
      assert(!f.open(path, io::file_base::read_write | io::file_base::truncate));
      auto [es2, sz2] = f.size();
      assert(!es2 && sz2 == 0);
      // create without exclusive opens an existing file
      assert(!f.open(path, io::file_base::read_write | io::file_base::create));
      // sync_all_on_write
      assert(!f.open(path, io::file_base::read_write | io::file_base::sync_all_on_write));
      auto [e6, n6] = co_await f.write_some_at(0, io::const_buffer(w.data(), w.size()));
      assert(!e6 && n6 == 10);
    }(f, dir / "d"));
  }

  // a closed file: bad_file_descriptor, everywhere
  {
    io::random_access_file f(ctx);
    assert(!f.open(dir / "e", io::file_base::read_write | io::file_base::create));
    f.close();
    f.close();
    assert(!f.is_open() && f.native_handle() == -1);
    assert(f.resize(1) == std::errc::bad_file_descriptor);
    assert(f.sync_data() == std::errc::bad_file_descriptor);
    assert(f.sync_all() == std::errc::bad_file_descriptor);
    assert(std::get<0>(f.size()) == std::errc::bad_file_descriptor);
    assert(std::get<0>(f.release()) == std::errc::bad_file_descriptor);
    run(ctx, [](io::random_access_file& f) -> io::task<void> {
      char c[4];
      auto [e1, n1] = co_await f.read_some_at(0, io::mutable_buffer(c, 4));
      assert(e1 == std::errc::bad_file_descriptor && n1 == 0);
      auto [e2, n2] = co_await f.write_some_at(0, io::const_buffer(c, 4));
      assert(e2 == std::errc::bad_file_descriptor && n2 == 0);
    }(f));
  }

  // moves, release, assign, std::filesystem::path
  {
    io::random_access_file f(ctx);
    assert(!f.open(std::filesystem::path(dir / "g"), io::file_base::read_write | io::file_base::create));
    int fd = f.native_handle();
    io::random_access_file g(std::move(f));
    assert(!f.is_open() && g.is_open() && g.native_handle() == fd);
    io::random_access_file h(ctx.get_executor());
    h = std::move(g);
    assert(!g.is_open() && h.is_open());
    auto [er, raw] = h.release();
    assert(!er && raw == fd && !h.is_open());
    assert(::fcntl(raw, F_GETFD) != -1); // still open
    assert(h.assign(-1) == std::errc::bad_file_descriptor);
    assert(!h.assign(raw));
    assert(h.is_open() && h.native_handle() == raw);
    h.close();
    assert(::fcntl(raw, F_GETFD) == -1); // closed by close()
    // opening a file closes the open one
    assert(!h.open(dir / "g", io::file_base::read_only));
    int fd1 = h.native_handle();
    assert(!h.open(dir / "g", io::file_base::read_only));
    (void)fd1;
  }
  return 0;
}
