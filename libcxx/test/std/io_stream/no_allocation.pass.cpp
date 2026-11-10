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

// The type-erasing wrappers allocate once, in the constructor, and never per operation: the number of calls to the
// global operator new is counted around the construction and around thousands of ready and suspending operations.

#include <io_stream>

#include <cassert>
#include <cstdlib>
#include <new>
#include <string>

#include "test_stream.h"

namespace io = std::io;
using namespace test_stream;
using test_io::test_context;

static int new_calls = 0;

void* operator new(std::size_t n) {
  ++new_calls;
  if (void* p = std::malloc(n ? n : 1))
    return p;
  TEST_THROW(std::bad_alloc());
}
void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }

io::task<void> loop_reads(io::any_read_stream& s, std::size_t* total) {
  char buf[64];
  for (;;) {
    auto [ec, n] = co_await s.read_some(io::mutable_buffer(buf, sizeof buf));
    *total += n;
    if (ec)
      co_return;
  }
}

io::task<void> loop_pipeline(io::any_stream& s, std::size_t* total) {
  char buf[16];
  for (int i = 0; i != 2000; ++i) {
    auto [ec, n] = co_await s.read_some(io::mutable_buffer(buf, sizeof buf));
    if (ec)
      break;
    auto [wec, wn] = co_await s.write_some(io::const_buffer(buf, n));
    assert(!wec && wn == n);
    *total += n;
  }
}

int main(int, char**) {
  // Ready operations: constructing the wrapper is the only allocation.
  {
    std::string payload(100000, 'q');
    memory_read_stream s{.data = payload, .chunk = 100};
    int before = new_calls;
    io::any_read_stream w(&s);
    assert(new_calls == before + 1);

    test_io::counting_resource frames; // the coroutine frames come from here, not from the global operator new
    test_context ctx;
    std::size_t total = 0;
    auto t            = loop_reads(w, &total);
    before            = new_calls;
    test_io::run_root(ctx, t, {}, &frames);
    assert(new_calls == before);
    assert(total == 100000);
    assert(s.calls == 1564); // 1563 reads of up to 64 bytes + the eof read
  }

  // Suspending operations in both directions through any_stream: two wrappers, hence two allocations; none after that.
  {
    std::string payload(40000, 'r');
    mock_tcp tcp;
    tcp.rd.data = payload;
    int before  = new_calls;
    io::any_stream w(&tcp);
    assert(new_calls == before + 2);
    tcp.wr.out.reserve(payload.size() + 16);

    test_io::counting_resource frames;
    test_context ctx;
    std::size_t total = 0;
    auto t            = loop_pipeline(w, &total);
    before            = new_calls;
    test_io::run_root(ctx, t, {}, &frames);
    assert(new_calls == before);
    assert(total == 32000 && tcp.wr.out.size() == 32000);
    assert(ctx.posted > 2000);
  }

  // Moving and re-wrapping does not allocate.
  {
    memory_read_stream s{.data = "x"};
    io::any_read_stream a(&s);
    int before = new_calls;
    io::any_read_stream b(std::move(a));
    a = std::move(b);
    assert(new_calls == before);
  }

  return 0;
}
