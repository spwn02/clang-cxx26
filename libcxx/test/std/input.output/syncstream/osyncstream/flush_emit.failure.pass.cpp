//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17
// UNSUPPORTED: no-localization
// UNSUPPORTED: no-threads
// UNSUPPORTED: availability-syncstream-missing

// <syncstream>

// LWG 3571: flush_emit(os) sets badbit if the emit() it performs fails.

#include <cassert>
#include <ostream>
#include <streambuf>
#include <syncstream>

// A stream buffer that never accepts any output.
struct FailingBuf : std::streambuf {
  std::streamsize xsputn(const char*, std::streamsize) override { return 0; }
  int_type overflow(int_type) override { return traits_type::eof(); }
};

struct WorkingBuf : std::streambuf {
  int written = 0;
  std::streamsize xsputn(const char*, std::streamsize n) override {
    written += static_cast<int>(n);
    return n;
  }
  int_type overflow(int_type c) override {
    ++written;
    return c;
  }
};

int main(int, char**) {
  {
    WorkingBuf wb;
    std::osyncstream os(&wb);
    os << "hello";
    assert(wb.written == 0); // nothing is emitted before flush_emit
    os << std::flush_emit;
    assert(os.good());
    assert(wb.written == 5);
  }
  {
    FailingBuf fb;
    std::osyncstream os(&fb);
    os << "hello";
    assert(os.good());
    os << std::flush_emit;
    assert(os.bad()); // the emit failed
  }
  {
    // A plain ostream is unaffected: there is nothing to emit.
    WorkingBuf wb;
    std::ostream os(&wb);
    os << "abc" << std::flush_emit;
    assert(os.good());
    assert(wb.written == 3);
  }
  return 0;
}
