//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// This test specifically needs debug info to exercise real DWARF resolution
// (source_file()/source_line()), unlike the rest of this directory. It also
// needs -rdynamic: description() resolves symbols via dladdr(), which for
// the main executable (unlike a shared library) only sees symbols in the
// dynamic symbol table -- without -rdynamic, an ordinary local function like
// capture_all() below wouldn't be dynamically exported at all, and this
// would be testing an artifact of the link line rather than the resolver.
// ADDITIONAL_COMPILE_FLAGS: -g -rdynamic

// <stacktrace>

// static basic_stacktrace current(allocator_type = allocator_type()) noexcept;
// static basic_stacktrace current(size_type skip, allocator_type = allocator_type()) noexcept;
// static basic_stacktrace current(size_type skip, size_type max_depth, allocator_type = allocator_type()) noexcept;
//
// This is the test that proves the from-scratch ELF/DWARF resolver actually
// works end to end (real stack capture via the platform unwinder, real
// symbol demangling via dladdr()/__cxa_demangle, real DWARF4/5 .debug_line
// parsing) -- not just that <stacktrace> compiles.

#include <cassert>
#include <cstdio>
#include <stacktrace>
#include <string>

#include "test_macros.h"

// All three current() overloads are called from this one function so their
// results are directly comparable frame-for-frame: each capture always
// starts at its own call site here (see basic_stacktrace::current()'s
// implementation note on why that's guaranteed regardless of optimization
// level), so index 0 always differs slightly between the three calls (three
// distinct call sites, three distinct lines below), but index 1 onward
// (main()'s call to capture_all(), and everything below main()) must be
// identical across all three captures. Each call site's own line is
// captured via __LINE__ right there, rather than hardcoded, so editing this
// file can't silently desynchronize the assertions below from reality.
unsigned full_line, skipped_line, truncated_line;
TEST_NOINLINE void capture_all(std::stacktrace& full, std::stacktrace& skipped, std::stacktrace& truncated) {
  full = std::stacktrace::current(); full_line = __LINE__;
  skipped = std::stacktrace::current(1); skipped_line = __LINE__;
  truncated = std::stacktrace::current(0, 3); truncated_line = __LINE__;
}

int main(int, char**) {
  std::stacktrace full, skipped, truncated;
  capture_all(full, skipped, truncated);

  assert(!full.empty());
  assert(skipped.size() == full.size() - 1);
  assert(truncated.size() == 3);

  // full[0] must be inside capture_all() itself, at its own call site.
  const std::stacktrace_entry& frame0 = full[0];
  assert(static_cast<bool>(frame0));
  assert(frame0.source_line() == full_line);
  std::string file0 = frame0.source_file();
  assert(!file0.empty());
  assert(file0.find("current.pass.cpp") != std::string::npos);
  assert(truncated[0].source_line() == truncated_line);
  (void)skipped_line;

  // Beyond each capture's own call site, every frame the three captures have
  // in common must agree exactly: skipping 1 from `full` must line up with
  // `skipped`'s own frame 0 onward, and likewise for `truncated`.
  assert(full[1] == skipped[0]);
  assert(full[1] == truncated[1]);
  assert(full.size() > 2);
  assert(full[2] == skipped[1]);

  // description() must resolve capture_all's own (exported, since it's not
  // static) mangled name into something human-readable containing its name.
  std::string desc = frame0.description();
  assert(desc.find("capture_all") != std::string::npos);

  std::printf(
      "stacktrace: %zu frames, frame0 = %s (%s:%u)\n", full.size(), desc.c_str(), file0.c_str(),
      (unsigned)frame0.source_line());

  return 0;
}
