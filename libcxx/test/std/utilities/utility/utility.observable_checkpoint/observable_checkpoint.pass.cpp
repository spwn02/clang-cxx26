//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// ADDITIONAL_COMPILE_FLAGS: -ffreestanding

#include <cassert>
#include <type_traits>
#include <utility>

#if __has_builtin(__builtin_observable_checkpoint)
#  ifndef __cpp_lib_observable_checkpoint
#    error "missing __cpp_lib_observable_checkpoint"
#  endif
static_assert(__cpp_lib_observable_checkpoint == 202506L);
static_assert(std::is_same_v<decltype(std::observable_checkpoint()), void>);
static_assert(noexcept(std::observable_checkpoint()));
#else
#  ifdef __cpp_lib_observable_checkpoint
#    error "__cpp_lib_observable_checkpoint requires compiler support"
#  endif
#endif

extern "C" int main() {
  int prefix = 7;
#if __has_builtin(__builtin_observable_checkpoint)
  std::observable_checkpoint();
#endif
  assert(prefix == 7);
  return 0;
}
