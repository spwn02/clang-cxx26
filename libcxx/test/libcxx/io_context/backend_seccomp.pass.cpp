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

// <io_context>

// The real refusal of io_uring: a seccomp filter (as in many container runtimes) makes io_uring_setup fail with EPERM.
// The io_context of the program must then run on epoll, report the reason, and the sockets must work. This exercises
// the failure path of the system call itself, which the test hook _LIBCPP_IO_URING_REFUSE of backend_selection.pass.cpp
// does not reach. The test does nothing where it cannot install the filter (another architecture, a sandbox).

#include <io>
#include <io_buffer>
#include <io_context>
#include <io_stream>
#include <net>

#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <linux/audit.h>
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <sys/prctl.h>
#include <unistd.h>

namespace io = std::io;

static bool deny_io_uring_setup() {
#if defined(__x86_64__)
  constexpr unsigned arch = AUDIT_ARCH_X86_64;
#elif defined(__aarch64__)
  constexpr unsigned arch = AUDIT_ARCH_AARCH64;
#else
  return false;
#endif
  constexpr unsigned nr_io_uring_setup = 425;
  sock_filter filter[] = {
      BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(seccomp_data, arch)),
      BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, arch, 1, 0),
      BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),
      BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(seccomp_data, nr)),
      BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, nr_io_uring_setup, 0, 1),
      BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ERRNO | EPERM),
      BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),
  };
  sock_fprog prog = {static_cast<unsigned short>(sizeof(filter) / sizeof(filter[0])), filter};
  if (::prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) != 0)
    return false;
  return ::prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &prog) == 0;
}

static io::task<void> echo_server(io::tcp_acceptor& acc) {
  auto [ec, sock] = co_await acc.accept();
  assert(!ec);
  char buf[16];
  auto [rec, n] = co_await sock.read_some(io::make_buffer(buf, sizeof buf));
  assert(!rec);
  auto [wec, wn] = co_await io::write(sock, io::make_buffer(static_cast<const void*>(buf), n));
  assert(!wec && wn == n);
}

static io::task<void> echo_client(io::io_context& ctx, io::endpoint server, bool* ok) {
  io::tcp_socket s(ctx);
  auto [cec] = co_await s.connect(server);
  assert(!cec);
  const char msg[] = "seccomp";
  auto [wec, wn]   = co_await io::write(s, io::const_buffer(msg, sizeof msg));
  assert(!wec && wn == sizeof msg);
  char back[16];
  auto [rec, rn] = co_await io::read(s, io::make_buffer(back, sizeof msg));
  assert(!rec && rn == sizeof msg && std::memcmp(back, msg, sizeof msg) == 0);
  *ok = true;
}

int main(int, char**) {
  ::unsetenv("_LIBCPP_IO_BACKEND");
  ::unsetenv("_LIBCPP_IO_URING_REFUSE");
  if (!deny_io_uring_setup()) {
    std::puts("cannot install a seccomp filter here: nothing to check");
    return 0;
  }
  for (const char* mode : {static_cast<const char*>(nullptr), "io_uring"}) {
    if (mode)
      ::setenv("_LIBCPP_IO_BACKEND", mode, 1);
    io::io_context ctx;
    auto& r = ctx.use_service<io::__reactor_service>();
    assert(std::strcmp(r.__backend(), "epoll") == 0 && !r.__has_ring());
    assert(r.__ring_refusal() == std::errc::operation_not_permitted);
    io::tcp_acceptor acc(ctx, io::endpoint(io::ipv4_address::loopback(), 0));
    bool ok = false;
    io::run_async(ctx.get_executor())(
        [](io::io_context& c, io::tcp_acceptor& a, bool* o) -> io::task<void> {
          co_await io::when_all(echo_server(a), echo_client(c, a.local_endpoint(), o));
        }(ctx, acc, &ok));
    ctx.run();
    assert(ok && r.__ring_transfers() == 0);
  }
  return 0;
}
