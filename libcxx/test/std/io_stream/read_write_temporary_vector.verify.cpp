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

// io::read and io::write hold a buffer sequence that is not a small trivially copyable value by reference, because the
// task is lazy: a temporary of such a type would dangle, and is rejected at compile time.

#include <io_stream>

#include <utility>
#include <vector>

#include "test_stream.h"

namespace io = std::io;
using namespace test_stream;

static std::vector<io::mutable_buffer> make_mutable();
static std::vector<io::const_buffer> make_const();

void f(memory_read_stream& r, memory_write_stream& w) {
  auto a = io::read(r, make_mutable());              // expected-error {{call to deleted function}}
  auto b = io::write(w, make_const());               // expected-error {{call to deleted function}}
  auto c = io::read(r, std::vector<io::mutable_buffer>()); // expected-error {{call to deleted function}}
  std::vector<io::const_buffer> v;
  auto d = io::write(w, v); // an lvalue is fine
  (void)a;
  (void)b;
  (void)c;
  (void)d;
}
