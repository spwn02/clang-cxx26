//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: no-threads

// RUN: %{cxx} %{flags} %{compile_flags} %s %{link_flags} -o %t.exe
// RUN: %{exec} %t.exe simple
// RUN: %{exec} %t.exe counting

#include <execution>
#include <exception>
#include <cstdlib>
#include <cstring>
// [exec.simple.counting.ctor]: "If state is not one of joined, unused, or
// unused-and-closed, invokes terminate. Otherwise, has no effects."
// [exec.scope.counting]: "Unless specified below, the semantics of members of
// counting_scope are the same as the corresponding members of simple_counting_scope."
template <class Scope>
void test() {
  Scope scope;
  auto token = scope.get_token();
  {
    auto assoc = token.try_associate();
    if (!assoc)
      std::_Exit(2);
  }
  // Also exercise the pre-RAII implementation with its count returned to zero.
  if constexpr (requires { token.disassociate(); })
    token.disassociate();
  // Scope is open, with zero associations: destruction must terminate.
}
int main(int argc, char** argv) {
  if (argc != 2)
    return 2;
  std::set_terminate([] { std::_Exit(0); });
  if (std::strcmp(argv[1], "simple") == 0)
    test<std::execution::simple_counting_scope>();
  else
    test<std::execution::counting_scope>();
  return 1;
}
