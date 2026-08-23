//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: no-localization

// class text_encoding
// [text.encoding.members]
//
//   static text_encoding environment();
//   template<id i> static bool environment_is();

#include <text_encoding>

#include <cassert>

using TE = std::text_encoding;
using ID = TE::id;

int main(int, char**) {
  // [text.encoding.members]p14: environment() is well-formed and returns
  // some text_encoding value representing the implementation-defined
  // execution environment encoding.
  TE env = TE::environment();

  // p18: environment_is<i>() == (environment() == i).
  assert(TE::environment_is<ID::unknown>() == (env == ID::unknown));

  // Calling twice is stable (not required verbatim by the wording beyond
  // "not affected by calls to setlocale", but a sane implementation
  // property worth pinning down).
  assert(TE::environment() == env);

  return 0;
}
