//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// REQUIRES: localization

#include <locale>
#include <text_encoding>
#include <type_traits>

static_assert(std::is_same_v<decltype(std::locale::classic().encoding()), std::text_encoding>);

int main(int, char**) {
  std::locale loc("C");
  (void)loc.encoding();
  return 0;
}
