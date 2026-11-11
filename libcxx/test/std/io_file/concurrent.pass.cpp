//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23, c++26
// UNSUPPORTED: no-threads
// REQUIRES: linux || darwin || freebsd || windows
// ADDITIONAL_COMPILE_FLAGS: -fexperimental-library
// <io_file>

// Concurrent file operations: many coroutines on one random_access_file and on separate files, run by four threads of one
// io_context; and the lifetime of the helper threads (a context with no operation, one that is destroyed after
// operations, a file that outlives nothing it needs).

#include <io_file>

#include <atomic>
#include <string>
#include <thread>
#include <vector>

#include "test_macros.h"
#include "test_file.h"

using namespace test_file;

constexpr int blocks     = 64;
constexpr int block_size = 4096;
constexpr int rounds     = 10;

static std::string pattern(int id, int round) {
  std::string s(block_size, static_cast<char>('a' + (id + round) % 26));
  s[0] = static_cast<char>(id);
  return s;
}

io::task<void> hammer(io::random_access_file& f, int id, std::atomic<int>& ok) {
  for (int round = 0; round != rounds; ++round) {
    std::string w = pattern(id, round);
    auto [e1, n1] = co_await f.write_some_at(std::uint64_t(id) * block_size, io::const_buffer(w.data(), w.size()));
    assert(!e1 && n1 == w.size());
    std::string r(block_size, '\0');
    auto [e2, n2] = co_await f.read_some_at(std::uint64_t(id) * block_size, io::mutable_buffer(r.data(), r.size()));
    assert(!e2 && n2 == r.size() && r == w);
  }
  ++ok;
}

io::task<void> own_file(io::io_context& ctx, std::string path, int id, std::atomic<int>& ok) {
  io::stream_file f(ctx);
  assert(!f.open(path, io::file_base::read_write | io::file_base::create | io::file_base::truncate));
  std::string w(100000, static_cast<char>('A' + id));
  std::size_t done = 0;
  while (done != w.size()) {
    auto [ec, n] = co_await f.write_some(io::const_buffer(w.data() + done, w.size() - done));
    assert(!ec);
    done += n;
  }
  (void)f.seek(0);
  std::string r;
  char buf[7000];
  for (;;) {
    auto [ec, n] = co_await f.read_some(io::mutable_buffer(buf, sizeof buf));
    if (ec == io::error::eof)
      break;
    assert(!ec);
    r.append(buf, n);
  }
  assert(r == w);
  ++ok;
}

int main(int, char**) {
  temp_dir dir;

  // a context that never had an operation
  {
    io::io_context ctx;
  }

  // a context destroyed after operations: the helper threads are joined, the files were closed
  {
    io::io_context ctx;
    io::stream_file f(ctx);
    assert(!f.open(dir / "x", io::file_base::write_only | io::file_base::create));
    run(ctx, [](io::stream_file& f) -> io::task<void> {
      auto [ec, n] = co_await f.write_some(io::const_buffer("x", 1));
      assert(!ec && n == 1);
    }(f));
  }

  // 4 threads: one shared random_access_file with 64 coroutines, and 8 separate files
  {
    io::io_context ctx;
    io::random_access_file shared(ctx);
    assert(!shared.open(dir / "shared", io::file_base::read_write | io::file_base::create));
    std::atomic<int> ok{0};
    for (int id = 0; id != blocks; ++id)
      io::run_async(ctx.get_executor(), [] {}, [](std::exception_ptr) { assert(false); })(hammer(shared, id, ok));
    for (int id = 0; id != 8; ++id)
      io::run_async(
          ctx.get_executor(), [] {}, [](std::exception_ptr) { assert(false); })(
          own_file(ctx, dir / ("own" + std::to_string(id)), id, ok));
    {
      watchdog wd(ctx, ms(15000));
      std::vector<std::thread> ts;
      for (int i = 0; i != 4; ++i)
        ts.emplace_back([&] { ctx.run(); });
      for (std::thread& t : ts)
        t.join();
    }
    assert(ok == blocks + 8);
    // the last round of every block is in the file
    auto [ec, sz] = shared.size();
    assert(!ec && sz == std::uint64_t(blocks) * block_size);
    shared.close();
  }
  return 0;
}
