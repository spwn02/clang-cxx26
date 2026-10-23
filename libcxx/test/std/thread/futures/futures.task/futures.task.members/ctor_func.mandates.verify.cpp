//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: no-threads
// UNSUPPORTED: c++03

// <future>

// template <class F>
//   explicit packaged_task(F&& f);
//
// LWG4154
// Mandates: is_invocable_r_v<R, decay_t<F>&, ArgTypes...> is true.

#include <future>

struct RvalueOnly {
  int operator()() && { return 0; }
};

void test() {
  // expected-error@*:* {{the callable must be invocable as an lvalue with the arguments of the signature}}
  // expected-error@*:* {{no matching function for call to '__invoke'}}
  std::packaged_task<int()> p{RvalueOnly{}};
}
