//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: clang-modules-build
// UNSUPPORTED: gcc

// XFAIL: has-no-cxx-module-support

// RUN: mkdir %t
// RUN: %{cxx} %{compile_flags} -std=c++26 -freflection-latest \
// RUN:     -Wno-reserved-module-identifier -Wno-reserved-user-defined-literal \
// RUN:     --precompile -o %t/std.pcm -c %{module-dir}/std.cppm
// RUN: %{cxx} %{compile_flags} %{link_flags} -std=c++26 -freflection-latest \
// RUN:     -fmodule-file=std=%t/std.pcm %t/std.pcm \
// RUN:     %s -o %t/std-memory-draft-api.sh.cpp.tsk
// RUN: %{exec} %t/std-memory-draft-api.sh.cpp.tsk

// [obj.lifetime], [specialized.construct], [smartptr]: facilities added to <memory> for the current draft, as seen
// through `import std;`.
#include <cassert>

import std;

struct Agg {
  int a;
  int b;
};

constexpr bool test() {
  // std::start_lifetime
  std::allocator<Agg> alloc;
  Agg* p = alloc.allocate(1);
  std::start_lifetime(*p);
  std::construct_at(&p->a, 1);
  std::construct_at(&p->b, 2);
  bool ok = p->a == 1 && p->b == 2;
  alloc.deallocate(p, 1);

  // construct_at of an array type
  std::allocator<int[2]> arrays;
  auto* q = arrays.allocate(1);
  std::construct_at(q);
  ok = ok && (*q)[0] == 0 && (*q)[1] == 0;
  std::destroy_at(q);
  arrays.deallocate(q, 1);

  // constexpr shared_ptr arrays of arrays and the overwrite factories
  auto md = std::make_shared<int[2][3]>();
  auto ow = std::make_shared_for_overwrite<int>();
  *ow     = 3;
  return ok && md[1][2] == 0 && *ow == 3;
}

int main(int, char**) {
  assert(test());
  static_assert(test());
  return 0;
}
