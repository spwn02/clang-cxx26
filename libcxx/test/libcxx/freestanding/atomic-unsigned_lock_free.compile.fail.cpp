// -*- C++ -*-
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// ADDITIONAL_COMPILE_FLAGS: -ffreestanding

// P1642R11 [atomics.syn]: std::atomic_unsigned_lock_free is not part of the
// freestanding subset -- whether it is defined at all is implementation-
// defined there per [compliance]p3, and this fork chooses not to provide it.

#include <atomic>

using __probe = std::atomic_unsigned_lock_free;
