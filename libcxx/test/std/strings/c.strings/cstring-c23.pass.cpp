//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// The C23 functions that C++26 adds to <cstring> and <cstdlib> ([cstring.syn], [cstdlib.syn]).

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <type_traits>

#include "test_macros.h"

int main(int, char**) {
  char dst[8] = {};
  void* r     = std::memccpy(dst, "abc.def", '.', sizeof(dst));
  assert(r == dst + 4 && std::strcmp(dst, "abc.") == 0);
  assert(std::memccpy(dst, "xyz", '#', 4) == nullptr);

  char secret[4] = {1, 2, 3, 4};
  assert(std::memset_explicit(secret, 0, sizeof(secret)) == secret);
  assert(secret[0] == 0 && secret[3] == 0);

  char* d = std::strdup("copy");
  assert(std::strcmp(d, "copy") == 0);
  std::free(d);
  char* n = std::strndup("truncate", 5);
  assert(std::strcmp(n, "trunc") == 0);
  std::free(n);

  alignas(64) char buf[64];
  assert(std::memalignment(buf) >= 64);
  assert(std::memalignment(nullptr) == 0);

  char out[32];
  assert(std::strfromd(out, sizeof(out), "%.2f", 1.5) == 4 && std::strcmp(out, "1.50") == 0);
  assert(std::strfromf(out, sizeof(out), "%.1f", 2.5f) == 3 && std::strcmp(out, "2.5") == 0);
  assert(std::strfroml(out, sizeof(out), "%.0f", 3.0L) == 1 && std::strcmp(out, "3") == 0);

  void* p = std::malloc(16);
  std::free_sized(p, 16);
  void* q = std::aligned_alloc(32, 64);
  std::free_aligned_sized(q, 32, 64);
  return 0;
}
