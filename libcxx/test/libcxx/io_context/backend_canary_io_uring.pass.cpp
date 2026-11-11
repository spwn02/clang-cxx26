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
// REQUIRES: io-backend=io_uring
// ADDITIONAL_COMPILE_FLAGS: -fexperimental-library

// <io_context>

// Canary for `--param io_backend=io_uring`: the request has to arrive in the test program (through the executor), and the
// io_context of the program has to be on the backend that was asked for. A sweep of the networking tests that does not
// run on the backend it claims to run on fails here.

#include <io_context>

#include <cassert>
#include <cstdlib>
#include <cstring>

int main(int, char**) {
  const char* requested = std::getenv("_LIBCPP_IO_BACKEND");
  assert(requested && std::strcmp(requested, "io_uring") == 0);
  std::io::io_context ctx;
  auto& r = ctx.use_service<std::io::__reactor_service>();
  // A kernel that refuses the ring leaves the context on epoll and says why; otherwise the ring is open.
  if (r.__has_ring())
    assert(std::strcmp(r.__backend(), "io_uring") == 0 && !r.__ring_refusal());
  else
    assert(std::strcmp(r.__backend(), "epoll") == 0 && r.__ring_refusal());
  return 0;
}
