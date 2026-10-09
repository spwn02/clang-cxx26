//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// REQUIRES: clang
// UNSUPPORTED: c++03

// <streambuf>

// The virtual function table of basic_streambuf is shared with the frozen C++03 headers (and with code compiled against
// earlier versions of the library): the standard virtual functions keep their slots, and anything the library adds
// has to come after them. Inserting a virtual function before them breaks all stream I/O of the frozen headers.

// RUN: %{cxx} -c %s %{flags} %{compile_flags} -o /dev/null -Xclang -fdump-vtable-layouts > %t.txt 2>&1
// RUN: sed -n "/Vtable for 'std::basic_streambuf'/,/^$/p" %t.txt | grep -oE '::[A-Za-z_~]+\(' | uniq > %t.names
// RUN: printf '::~basic_streambuf(\n::imbue(\n::setbuf(\n::seekoff(\n::seekpos(\n::sync(\n::showmanyc(\n::xsgetn(\n::underflow(\n::uflow(\n::pbackfail(\n::xsputn(\n::overflow(\n::__set_emit_on_sync(\n::__emit_on_flush(\n' > %t.expected
// RUN: diff %t.expected %t.names

#include <streambuf>

struct Derived : std::streambuf {
  int_type overflow(int_type) override { return 0; }
};
Derived d;
