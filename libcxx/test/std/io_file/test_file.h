//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef TEST_STD_IO_FILE_TEST_FILE_H
#define TEST_STD_IO_FILE_TEST_FILE_H

#include <cassert>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <io>
#include <io_buffer>
#include <io_context>
#include <io_file>
#include <io_stream>
#include <string>
#include <system_error>
#include <utility>
#include <csignal>
#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  define NOMINMAX
#  include <windows.h>
#else
#  include <signal.h>
#  include <unistd.h>
#endif

#include "test_ctx.h"

namespace test_file {

namespace io = std::io;
using namespace test_ctx;

// Ignores SIGPIPE: a write to a FIFO whose reader is gone must not kill the test. Written with sigaction and a cast
// through the type of the member because SIG_IGN has C++ language linkage on macOS and the parameter of signal() has C
// language linkage (distinct function types in this language).
inline void ignore_sigpipe() {
#ifndef _WIN32
  using handler_t = decltype(std::declval<struct sigaction&>().sa_handler);
  struct sigaction sa;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags   = 0;
  sa.sa_handler = reinterpret_cast<handler_t>(SIG_IGN);
  ::sigaction(SIGPIPE, &sa, nullptr);
#endif
}

// Runs the task on the context (with a watchdog that stops a hung test) until the context has no more work.
inline void run(io::io_context& ctx, io::task<void> t, ms limit = ms(8000)) {
  ctx.restart(); // a context that ran out of work is stopped
  bool done = false;
  io::run_async(
      ctx.get_executor(), [&] { done = true; }, [](std::exception_ptr) { assert(false && "task failed"); })(
      std::move(t));
  watchdog wd(ctx, limit);
  ctx.run();
  assert(done);
}

// A directory below TMPDIR (the temporary directory of the test), removed with its files at the end.
struct temp_dir {
  std::string path;
  temp_dir() {
#ifdef _WIN32
    static int counter = 0;
    std::filesystem::path base = std::filesystem::temp_directory_path();
    for (;;) {
      std::filesystem::path p = base / ("libcxxfile" + std::to_string(::GetCurrentProcessId()) + "_" + std::to_string(counter++));
      std::error_code ec;
      if (std::filesystem::create_directory(p, ec)) {
        path = p.generic_string();
        break;
      }
      assert(!ec);
    }
#else
    const char* base = std::getenv("TMPDIR");
    std::string tmpl = std::string(base && *base ? base : "/tmp") + "/fileXXXXXX";
    char* r          = ::mkdtemp(tmpl.data());
    assert(r);
    path = r;
#endif
  }
  ~temp_dir() {
    std::error_code ec;
    std::filesystem::remove_all(path, ec);
  }
  std::string operator/(const std::string& name) const { return path + "/" + name; }
};

inline io::const_buffer cbuf(const std::string& s) { return io::const_buffer(s.data(), s.size()); }

} // namespace test_file

#endif // TEST_STD_IO_FILE_TEST_FILE_H
