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

// <net>

// The resolver and the file operations of a context do not share their helper threads: file reads that are blocked
// (FIFOs with nothing to read) on every file thread must not delay a lookup.

#include <io_file>
#include <net>

#include <atomic>
#include <cassert>
#include <csignal>
#include <cstdio>
#include <chrono>
#include <fcntl.h>
#include <string>
#include <thread>
#include <vector>
#include <sys/stat.h>
#include <unistd.h>

#include "test_macros.h"
#include "test_net.h"

namespace io = std::io;
using namespace std::chrono_literals;

struct temp_fifos {
  std::string dir;
  temp_fifos() {
    char tmpl[] = "/tmp/lanesXXXXXX";
    assert(::mkdtemp(tmpl));
    dir = tmpl;
  }
  ~temp_fifos() {
    for (int i = 0; i != 8; ++i)
      ::unlink((dir + "/f" + std::to_string(i)).c_str());
    ::rmdir(dir.c_str());
  }
};

io::task<void> blocked_read(io::stream_file& f) {
  char c[2];
  (void)co_await f.read_some(io::mutable_buffer(c, 1));
}

io::task<void> lookup(io::io_context& ctx, std::atomic<bool>* done) {
  io::resolver r(ctx);
  auto [ec, res] = co_await r.resolve("127.0.0.1", "80", io::resolve_flags::numeric_service);
  assert(!ec && res.size() >= 1);
  *done = true;
}

// Destroyed right before the context: the destruction starts now, the writer releases the blocked reads.
struct destruction_starts {
  std::atomic<bool>* flag;
  ~destruction_starts() { flag->store(true); }
};

int main(int, char**) {
  ::signal(SIGPIPE, SIG_IGN);
  temp_fifos t;
  constexpr int K = 5; // more than the four file threads
  int raw[K];
  std::atomic<bool> resolved{false}, destroying{false}, finished{false};
  std::thread writer;
  {
    io::io_context ctx;
    std::vector<io::stream_file> fifo;
    for (int i = 0; i != K; ++i)
      fifo.emplace_back(ctx);
    for (int i = 0; i != K; ++i) {
      std::string p = t.dir + "/f" + std::to_string(i);
      assert(::mkfifo(p.c_str(), 0600) == 0);
      assert(!fifo[i].open(p, io::file_base::read_write));
      raw[i] = ::open(p.c_str(), O_WRONLY | O_NONBLOCK);
      assert(raw[i] >= 0);
    }
    for (int i = 0; i != K; ++i)
      io::run_async(ctx.get_executor())(blocked_read(fifo[i]));
    ctx.poll(); // four file threads are blocked in read(), one request waits for a thread
    std::this_thread::sleep_for(100ms);

    io::run_async(ctx.get_executor())(lookup(ctx, &resolved));
    auto limit = std::chrono::steady_clock::now() + 5s;
    while (!resolved && std::chrono::steady_clock::now() < limit) {
      ctx.poll();
      std::this_thread::sleep_for(1ms);
    }
    writer = std::thread([&] {
      while (!destroying)
        std::this_thread::sleep_for(1ms);
      while (!finished) {
        for (int i = 0; i != K; ++i)
          (void)::write(raw[i], "x", 1);
        std::this_thread::sleep_for(2ms);
      }
    });
    destruction_starts starts{&destroying};
  }
  finished = true;
  writer.join();
  for (int i = 0; i != K; ++i)
    ::close(raw[i]);
  assert(resolved && "the lookup was starved by the blocked file reads");
  return 0;
}
