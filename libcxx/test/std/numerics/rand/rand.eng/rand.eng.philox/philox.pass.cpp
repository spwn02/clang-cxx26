//===----------------------------------------------------------------------===//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: no-localization

#include <array>
#include <cassert>
#include <random>
#include <sstream>

#include "test_macros.h"

template <class Engine>
void test_engine(typename Engine::result_type expected) {
  static_assert(Engine::word_count == 4);
  static_assert(Engine::round_count == 10);
  static_assert(Engine::min() == 0);
  static_assert(Engine::min() < Engine::max());

  Engine e;
  for (int i = 0; i != 9999; ++i)
    (void)e();
  assert(e() == expected); // [rand.predef]'s required reference output.

  Engine a(17), b(17);
  a.discard(37);
  for (int i = 0; i != 37; ++i)
    (void)b();
  assert(a == b);
  assert(a != Engine(18));

  std::ostringstream os;
  os << a;
  Engine restored;
  std::istringstream is(os.str());
  is >> restored;
  assert(restored == a);
  for (int i = 0; i != 8; ++i)
    assert(restored() == a());
}

int main(int, char**) {
  test_engine<std::philox4x32>(1955073260u);
  test_engine<std::philox4x64>(3409172418970261260ULL);

  std::philox4x32 a(42), b(42);
  a.set_counter({0, 0, 0, 7});
  b.discard(7 * 4);
  for (int i = 0; i != 4; ++i)
    assert(a() == b());
}
