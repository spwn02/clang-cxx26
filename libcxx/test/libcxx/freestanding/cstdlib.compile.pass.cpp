// -*- C++ -*-
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// UNSUPPORTED: c++03, c++11, c++14, c++17
// ADDITIONAL_COMPILE_FLAGS: -ffreestanding

// P1642R11 [cstdlib.syn]: size_t, NULL, abort, atexit, at_quick_exit, exit,
// _Exit, and quick_exit are freestanding. This fork's <cstdlib> exposes its
// whole C-library surface unconditionally (nothing there is hosted-only), so
// this is a positive-availability probe, not a gating test.

#include <cstdlib>

#include "test_macros.h"

#ifndef _LIBCPP_FREESTANDING
#  error "-ffreestanding must select libc++ freestanding mode"
#endif

using size_type = std::size_t;
static_assert(sizeof(size_type) == sizeof(decltype(sizeof(0))), "");

void handler() {}

void test_freestanding_cstdlib() {
  void* __null_ptr = NULL;
  (void)__null_ptr;
  (void)std::abort;
  (void)std::atexit(&handler);
  (void)std::at_quick_exit(&handler);
  (void)std::exit;
  (void)std::_Exit;
  (void)std::quick_exit;
}
