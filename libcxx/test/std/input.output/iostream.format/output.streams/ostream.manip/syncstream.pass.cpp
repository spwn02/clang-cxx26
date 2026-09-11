//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17
// UNSUPPORTED: no-localization
// UNSUPPORTED: libcpp-has-no-experimental-syncstream

// <ostream>

// template <class charT, class traits>
// basic_ostream<charT, traits>& emit_on_flush(basic_ostream<charT, traits>&);
// template <class charT, class traits>
// basic_ostream<charT, traits>& noemit_on_flush(basic_ostream<charT, traits>&);
// template <class charT, class traits>
// basic_ostream<charT, traits>& flush_emit(basic_ostream<charT, traits>&);

#include <ostream>
#include <sstream>
#include <syncstream>
#include <cassert>

#include "test_macros.h"

template <class CharT>
void test() {
  std::basic_ostringstream<CharT> wrapped;
  std::basic_osyncstream<CharT> os(wrapped);

  os << std::emit_on_flush << CharT('a');
  os.flush();
  assert(wrapped.str() == std::basic_string<CharT>(1, CharT('a')));

  os << std::noemit_on_flush << CharT('b');
  os.flush();
  assert(wrapped.str() == std::basic_string<CharT>(1, CharT('a')));

  os << std::flush_emit;
  assert(wrapped.str() == std::basic_string<CharT>{CharT('a'), CharT('b')});

  // The manipulators have no syncbuf-specific effect on ordinary streams.
  std::basic_ostringstream<CharT> ordinary;
  ordinary << std::emit_on_flush << std::noemit_on_flush << std::flush_emit;
  assert(ordinary.good());
}

int main(int, char**) {
  test<char>();
#ifndef TEST_HAS_NO_WIDE_CHARACTERS
  test<wchar_t>();
#endif

  return 0;
}
