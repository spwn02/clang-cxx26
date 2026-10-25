//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: no-threads
// REQUIRES: linux

#include <assert.h>
#include <cxxabi.h>

extern "C" { typedef void (*Dtor_t)(void *); }

static bool AtexitImplCalled = false;

extern "C" int __cxa_thread_atexit_impl(Dtor_t dtor, void *obj, void *dso_symbol) {
  assert(dtor == reinterpret_cast<Dtor_t>(1));
  assert(obj == reinterpret_cast<void *>(2));
  assert(dso_symbol == reinterpret_cast<void *>(3));
  AtexitImplCalled = true;
  return 4;
}

int main(int, char**) {
  int RV = __cxxabiv1::__cxa_thread_atexit(
      reinterpret_cast<Dtor_t>(1), reinterpret_cast<void *>(2),
      reinterpret_cast<void *>(3));
  assert(RV == 4);
  assert(AtexitImplCalled);
  return 0;
}
