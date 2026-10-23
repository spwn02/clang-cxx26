//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14

// <memory>

// LWG3918: uninitialized_move and uninitialized_move_n use deref-move, so an input
// iterator that returns a prvalue of a type without move operations can be used.

#include <memory>
#include <cassert>
#include <iterator>
struct Imm { int v; Imm(int x) : v(x) {} Imm(Imm&&) = delete; Imm(const Imm&) = delete; };
struct It {
  using difference_type = std::ptrdiff_t; using value_type = Imm; using iterator_category = std::input_iterator_tag;
  int i = 0;
  Imm operator*() const { return Imm(i); }
  It& operator++() { ++i; return *this; } It operator++(int) { auto t = *this; ++i; return t; }
  friend bool operator==(const It& a, const It& b) { return a.i == b.i; }
};
int main(int, char**) {
  alignas(Imm) unsigned char buf[3*sizeof(Imm)];
  Imm* out = reinterpret_cast<Imm*>(buf);
  std::uninitialized_move(It{0}, It{3}, out);
  assert(out[2].v == 2);
  alignas(Imm) unsigned char buf2[3*sizeof(Imm)];
  std::uninitialized_move_n(It{0}, 3, reinterpret_cast<Imm*>(buf2));
  return 0;
}
