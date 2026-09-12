//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// test runtime_error

#include <stdexcept>
#include <type_traits>
#include <cstring>
#include <string>
#include <cassert>

#include "test_macros.h"

#if TEST_STD_VER > 23
constexpr bool test_constexpr_runtime_error() {
  char msg[] = "runtime_error message";
  std::runtime_error e(msg);
  msg[0] = 'x'; // Constant evaluation must own its copy, not retain msg.
  std::runtime_error copy(e);
  copy = e;
  return copy.what()[0] == 'r' && copy.what()[21] == '\0';
}

static_assert(test_constexpr_runtime_error());
#endif

int main(int, char**)
{
    static_assert((std::is_base_of<std::exception, std::runtime_error>::value),
                 "std::is_base_of<std::exception, std::runtime_error>::value");
    static_assert(std::is_polymorphic<std::runtime_error>::value,
                 "std::is_polymorphic<std::runtime_error>::value");
    {
    const char* msg = "runtime_error message";
    std::runtime_error e(msg);
    assert(std::strcmp(e.what(), msg) == 0);
    std::runtime_error e2(e);
    assert(std::strcmp(e2.what(), msg) == 0);
    e2 = e;
    assert(std::strcmp(e2.what(), msg) == 0);
    }
    {
    std::string msg("another runtime_error message");
    std::runtime_error e(msg);
    assert(e.what() == msg);
    std::runtime_error e2(e);
    assert(e2.what() == msg);
    e2 = e;
    assert(e2.what() == msg);
    }

  return 0;
}
