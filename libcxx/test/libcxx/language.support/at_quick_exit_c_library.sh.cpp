//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17

// The std overloads of at_quick_exit for callbacks with C and C++ language linkage (__cstdlib/callbacks.h) work with
// every C library: where the C library declares at_quick_exit for a callback with C++ linkage (glibc) or with C
// linkage (the BSDs), both kinds of callback are accepted; where it has none (Apple's before macOS 15) the header
// still compiles and std::at_quick_exit is simply not callable.

// RUN: %{cxx} %s -I %S/Inputs/fake_libc %{flags} %{compile_flags} -fsyntax-only -DVARIANT=0
// RUN: %{cxx} %s -I %S/Inputs/fake_libc %{flags} %{compile_flags} -fsyntax-only -DVARIANT=1
// RUN: %{cxx} %s -I %S/Inputs/fake_libc %{flags} %{compile_flags} -fsyntax-only -DVARIANT=2

#include <__config>
#include <__cstdlib/callbacks.h>

extern "C" void c_callback() {}
void cxx_callback() {}

template <class F>
concept can_register = requires(F f) { std::at_quick_exit(f); };

extern "C" {
typedef void c_function();
}

#if VARIANT == 0
static_assert(!can_register<void (*)()> && !can_register<c_function*>);
#else
static_assert(can_register<void (*)()> && can_register<c_function*>);

int register_all() {
  return std::at_quick_exit(c_callback) + std::at_quick_exit(cxx_callback) + std::at_quick_exit([] {});
}
#endif
