//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: no-threads

// <thread>

// [thread.thread.constr], [thread.jthread.cons]: Mandates for thread(attrs..., f, args...) and jthread(attrs..., f, args...)

#include <thread>

// expected-note@*:* 0+ {{}}

void f() {}

void test() {
  // the attributes have to be followed by a function
  std::thread a(std::thread::stack_size_hint(1)); // expected-error@*:* {{Mandates: a function to invoke follows the thread attributes}}
  std::jthread b(std::thread::stack_size_hint(1)); // expected-error@*:* {{Mandates: a function to invoke follows the thread attributes}}

  // no attribute type more than once
  std::thread c(std::thread::stack_size_hint(1), std::thread::stack_size_hint(2), f); // expected-error@*:* {{Mandates: no thread attribute type is present more than once}}
}
