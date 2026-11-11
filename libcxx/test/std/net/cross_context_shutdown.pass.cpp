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

// A chain that spans two contexts (run(other_executor)(task)) is destroyed by the context of its root. When that context
// is destroyed while the chain waits in the other one, the operation that is pending there is unlinked by the
// destructor of its awaiter: the other context holds no pointer into the freed frame, does not count the wait as work
// (it runs out of work and stops), and the destruction of the other context is safe.

#include <io_file>
#include <io_signal>
#include <io_timer>
#include <net>

#include <atomic>
#include <cassert>
#include <chrono>
#include <csignal>
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

struct sentinel {
  std::atomic<int>* n;
  explicit sentinel(std::atomic<int>* p) : n(p) {}
  sentinel(sentinel&& o) noexcept : n(std::exchange(o.n, nullptr)) {}
  ~sentinel() {
    if (n)
      ++*n;
  }
};


constexpr int K = 5; // more than the four file threads
std::string fifo_path[K];
int fifo_fd[K];

using maker_t = io::task<void> (*)(io::io_context&, sentinel, int);

io::task<void> wait_timer(io::io_context& b, sentinel, int) {
  io::timer t(b, 1h);
  (void)co_await t.wait();
}

io::task<void> wait_signal(io::io_context& b, sentinel, int) {
  io::signal_set s(b);
  assert(!s.add(SIGUSR2, io::signal_set::dont_care));
  (void)co_await s.wait();
}

io::task<void> wait_accept(io::io_context& b, sentinel, int) {
  io::tcp_acceptor acc(b, test_net::loopback());
  (void)co_await acc.accept();
}

io::task<void> recv_socket(io::io_context& b, sentinel, int) {
  io::tcp_acceptor acc(b, test_net::loopback());
  io::tcp_socket client(b), server(b);
  auto [r1, r2] = co_await io::when_all(
      [](io::tcp_acceptor& a) -> io::task<io::tcp_socket> {
        auto [ec, s] = co_await a.accept();
        assert(!ec);
        co_return std::move(s);
      }(acc),
      client.connect(acc.local_endpoint()));
  assert(!std::get<0>(r2));
  server = std::move(r1);
  char c[4];
  (void)co_await client.read_some(io::mutable_buffer(c, 1)); // nothing is sent: a request on the ring, or a readiness wait
}

io::task<void> read_fifo(io::io_context& b, sentinel, int i) {
  io::stream_file f(b);
  assert(!f.open(fifo_path[i], io::file_base::read_write));
  char c[4];
  (void)co_await f.read_some(io::mutable_buffer(c, 1));
}

// The root runs on a, the awaited chain on b.
io::task<void> root(io::io_context& b, maker_t make, int i, std::atomic<int>* d) {
  co_await io::run(b.get_executor())(make(b, sentinel(d), i));
}

// Destroyed right before a (declared after it): the destruction of a starts now.
struct destruction_starts {
  std::atomic<bool>* flag;
  ~destruction_starts() { flag->store(true); }
};

// Runs one scenario: n chains are suspended in b, a is destroyed (while the writer releases the calls in progress, if
// any), and b is then left without work.
void scenario(maker_t make, int n) {
  std::atomic<int> d{0};
  std::atomic<bool> destroying{false}, finished{false};
  std::thread writer([&] {
    while (!destroying)
      std::this_thread::sleep_for(1ms);
    while (!finished) {
      for (int i = 0; i != K; ++i)
        if (fifo_fd[i] >= 0)
          (void)::write(fifo_fd[i], "x", 1);
      std::this_thread::sleep_for(2ms);
    }
  });
  io::io_context b;
  {
    io::io_context a;
    for (int i = 0; i != n; ++i)
      io::run_async(a.get_executor())(root(b, make, i, &d));
    a.poll(); // starts the roots, which post the inner chains to b
    b.poll(); // runs the inner chains up to their waits
    std::this_thread::sleep_for(50ms); // lets the helper threads reach their calls
    b.poll();
    assert(d == 0);
    destruction_starts starts{&destroying};
  } // a destroys the chains; the waits that are pending in b are unlinked
  finished = true;
  writer.join();
  assert(d == n);
  b.restart();
  assert(b.poll() == 0);
  assert(b.stopped()); // nothing is pending any more: the waits were not left counted as work
}

int main(int, char**) {
  ::signal(SIGPIPE, SIG_IGN);
  char tmpl[] = "/tmp/xctxXXXXXX";
  assert(::mkdtemp(tmpl));
  for (int i = 0; i != K; ++i) {
    fifo_path[i] = std::string(tmpl) + "/f" + std::to_string(i);
    assert(::mkfifo(fifo_path[i].c_str(), 0600) == 0);
    fifo_fd[i] = -1;
  }
  scenario(&wait_timer, 3);
  scenario(&wait_signal, 1);
  scenario(&wait_accept, 3);
  scenario(&recv_socket, 2);
  for (int i = 0; i != K; ++i)
    fifo_fd[i] = ::open(fifo_path[i].c_str(), O_RDWR | O_NONBLOCK);
  scenario(&read_fifo, 1); // the call is in progress on a helper thread
  scenario(&read_fifo, K); // and one request waits for a thread
  for (int i = 0; i != K; ++i) {
    ::close(fifo_fd[i]);
    ::unlink(fifo_path[i].c_str());
  }
  ::rmdir(tmpl);
  return 0;
}
