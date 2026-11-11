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

// An io_context that is destroyed while file operations are pending destroys the chains of the coroutines that wait.
// The helper threads are blocked in read() of FIFOs; another thread writes to the FIFOs shortly after the destruction
// has started, so that the calls in progress return and the destructor, which waits for them, can finish. The
// operations that wait for a thread are dropped without ever calling the system.

#include <io_file>

#include <atomic>
#include <cassert>
#include <csignal>
#include <fcntl.h>
#include <thread>
#include <vector>
#include <sys/stat.h>
#include <unistd.h>

#include "test_macros.h"
#include "test_file.h"

using namespace test_file;

struct sentinel {
  std::atomic<int>* n;
  explicit sentinel(std::atomic<int>* p) : n(p) {}
  sentinel(sentinel&& o) noexcept : n(std::exchange(o.n, nullptr)) {}
  ~sentinel() {
    if (n)
      ++*n;
  }
};

io::task<void> read_one(io::stream_file& f, std::atomic<int>* reads_done, sentinel) {
  char c[4];
  (void)co_await f.read_some(io::mutable_buffer(c, 1));
  ++*reads_done;
}

int main(int, char**) {
  ::signal(SIGPIPE, SIG_IGN);
  temp_dir dir;
  constexpr int K = 6; // more readers than helper threads (four)
  int raw[K];
  std::atomic<int> d{0}, reads_done{0};
  std::thread writer;
  {
    io::io_context ctx;
    std::vector<io::stream_file> fifo;
    for (int i = 0; i != K; ++i)
      fifo.emplace_back(ctx);
    for (int i = 0; i != K; ++i) {
      std::string p = dir / ("fifo" + std::to_string(i));
      assert(::mkfifo(p.c_str(), 0600) == 0);
      assert(!fifo[i].open(p, io::file_base::read_write)); // does not block
      raw[i] = ::open(p.c_str(), O_WRONLY | O_NONBLOCK);
      assert(raw[i] >= 0);
    }
    for (int i = 0; i != K; ++i)
      io::run_async(ctx.get_executor())(read_one(fifo[i], &reads_done, sentinel(&d)));
    // starts the readers: four threads are blocked in read(), two requests wait for a thread
    ctx.poll();
    std::this_thread::sleep_for(ms(100));
    ctx.poll();
    assert(d == 0 && reads_done == 0);
    writer = std::thread([&] {
      std::this_thread::sleep_for(ms(150)); // after the destruction has started and waits for the threads
      // The last two FIFOs have no reader once the destructor has dropped the requests that waited for a thread, so the
      // write may legitimately fail with EPIPE: it must not kill the process (SIGPIPE is ignored) nor be asserted on.
      for (int i = 0; i != K; ++i)
        (void)::write(raw[i], "x", 1);
    });
    // the files go first, then the context, which waits for the calls in progress
  }
  writer.join();
  assert(reads_done == 0); // no coroutine was resumed
  assert(d == K);          // every chain was destroyed
  for (int i = 0; i != K; ++i)
    ::close(raw[i]);
  return 0;
}
