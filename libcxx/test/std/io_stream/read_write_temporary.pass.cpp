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

// io::read and io::write return lazy tasks. A buffer, a pair of buffers or another small trivially copyable sequence is
// copied into the task, so the natural spelling io::read(s, io::make_buffer(p, n)) is fine although the temporary is
// gone when the task runs (the task used to hold a dangling reference). A larger sequence (a vector of buffers) is held
// by reference and must outlive the task, so a temporary of such a type is rejected (see the .verify.cpp) and an lvalue
// is accepted.

#include <io_stream>

#include <array>
#include <cassert>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "test_stream.h"

namespace io = std::io;
using namespace test_stream;
using test_io::test_context;

// What may be passed to read() and write(), as expressions of the overload set.
template <class S, class Seq>
concept can_read = requires(S& s, Seq&& q) { io::read(s, std::forward<Seq>(q)); };
template <class S, class Seq>
concept can_write = requires(S& s, Seq&& q) { io::write(s, std::forward<Seq>(q)); };

static_assert(can_read<memory_read_stream, io::mutable_buffer>);
static_assert(can_read<memory_read_stream, io::mutable_buffer&>);
static_assert(can_read<memory_read_stream, io::mutable_buffer_pair>);
static_assert(can_read<memory_read_stream, std::vector<io::mutable_buffer>&>);
static_assert(can_read<memory_read_stream, const std::vector<io::mutable_buffer>&>);
static_assert(!can_read<memory_read_stream, std::vector<io::mutable_buffer>>);
static_assert(!can_read<memory_read_stream, std::vector<io::mutable_buffer>&&>);
static_assert(can_write<memory_write_stream, io::const_buffer>);
static_assert(can_write<memory_write_stream, io::const_buffer_pair>);
static_assert(can_write<memory_write_stream, std::vector<io::const_buffer>&>);
static_assert(!can_write<memory_write_stream, std::vector<io::const_buffer>>);
// An array of buffers is held by reference when it is big, copied when it is small, and an lvalue array works.
static_assert(can_read<memory_read_stream, io::mutable_buffer (&)[3]>);
static_assert(can_read<memory_read_stream, io::mutable_buffer (&)[2]>);
static_assert(!can_read<memory_read_stream, std::array<io::mutable_buffer, 5>>);
static_assert(can_read<memory_read_stream, std::array<io::mutable_buffer, 5>&>);

struct result_t {
  std::error_code ec;
  std::size_t n = 0;
};

// The temporaries of the arguments are gone when these return.
static io::task<io::io_result<std::size_t>> start_read(memory_read_stream& s, char* p, std::size_t n) {
  return io::read(s, io::make_buffer(p, n));
}
static io::task<io::io_result<std::size_t>> start_read_pair(memory_read_stream& s, char* p, std::size_t n) {
  return io::read(s, io::mutable_buffer_pair{io::mutable_buffer(p, n / 2), io::mutable_buffer(p + n / 2, n - n / 2)});
}
static io::task<io::io_result<std::size_t>> start_write(memory_write_stream& s, const char* p, std::size_t n) {
  return io::write(s, io::make_buffer(p, n));
}
static io::task<io::io_result<std::size_t>> start_write_pair(memory_write_stream& s, const char* p, std::size_t n) {
  return io::write(s, io::const_buffer_pair{io::const_buffer(p, n / 2), io::const_buffer(p + n / 2, n - n / 2)});
}

// Overwrites the part of the stack that the temporaries of the callers occupied.
[[clang::noinline]] static void clobber() {
  volatile char junk[16384];
  for (std::size_t i = 0; i != sizeof junk; ++i)
    junk[i] = static_cast<char>(0x5a);
}

static io::task<void> drive(io::task<io::io_result<std::size_t>> t, result_t* r) {
  auto [ec, n] = co_await std::move(t);
  r->ec        = ec;
  r->n         = n;
}

static result_t run(io::task<io::io_result<std::size_t>> inner) {
  clobber();
  test_context ctx;
  result_t r;
  auto d = drive(std::move(inner), &r);
  test_io::run_root(ctx, d);
  return r;
}

int main(int, char**) {
  using namespace std::string_view_literals;
  {
    char buf[8] = {};
    memory_read_stream s;
    s.data = "abcdefgh"sv;
    result_t r = run(start_read(s, buf, 8));
    assert(!r.ec && r.n == 8 && std::string(buf, 8) == "abcdefgh");
  }
  {
    char buf[9] = {};
    memory_read_stream s;
    s.data  = "abcdefghi"sv;
    s.chunk = 2;
    result_t r = run(start_read_pair(s, buf, 9));
    assert(!r.ec && r.n == 9 && std::string(buf, 9) == "abcdefghi");
  }
  {
    memory_write_stream s;
    s.chunk = 3;
    result_t r = run(start_write(s, "0123456789", 10));
    assert(!r.ec && r.n == 10 && s.out == "0123456789");
  }
  {
    memory_write_stream s;
    s.chunk = 4;
    result_t r = run(start_write_pair(s, "ABCDEFGHIJK", 11));
    assert(!r.ec && r.n == 11 && s.out == "ABCDEFGHIJK");
  }
  // An lvalue vector of buffers, and a lvalue buffer, still work (the vector is held by reference).
  {
    std::string src = "0123456789";
    std::vector<io::const_buffer> v{io::const_buffer(src.data(), 4), io::const_buffer(src.data() + 4, 6)};
    memory_write_stream s;
    s.chunk = 5;
    result_t r = run(io::write(s, v));
    assert(!r.ec && r.n == 10 && s.out == src);
    io::const_buffer one(src.data(), 3);
    memory_write_stream s2;
    r = run(io::write(s2, one));
    assert(!r.ec && r.n == 3 && s2.out == "012");
  }
  return 0;
}
