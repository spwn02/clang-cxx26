//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23, c++26

// P3154R3: the iostream overloads that take signed char / unsigned char (and
// pointers and arrays of them) are deprecated; the char and int overloads are not.

#include <istream>
#include <ostream>
#include <sstream>

void test() {
  std::istringstream in("abc");
  std::ostringstream out;

  signed char sc        = 'a';
  unsigned char uc      = 'b';
  signed char sa[4]     = {};
  unsigned char ua[4]   = {};
  const signed char* sp = sa;
  const unsigned char* up = ua;

  // Each use is diagnosed at least once.
  // expected-warning@+1 1+ {{is deprecated}}
  out << sc;
  // expected-warning@+1 1+ {{is deprecated}}
  out << uc;
  // expected-warning@+1 1+ {{is deprecated}}
  out << sp;
  // expected-warning@+1 1+ {{is deprecated}}
  out << up;
  // expected-warning@+1 1+ {{is deprecated}}
  in >> sc;
  // expected-warning@+1 1+ {{is deprecated}}
  in >> uc;
  // expected-warning@+1 1+ {{is deprecated}}
  in >> sa;
  // expected-warning@+1 1+ {{is deprecated}}
  in >> ua;

  // Not deprecated: char, integers and the explicitly cast forms.
  char c = 'c';
  char ca[4];
  out << c << static_cast<int>(sc) << static_cast<int>(uc) << reinterpret_cast<const char*>(sp);
  in >> c >> ca;
}
