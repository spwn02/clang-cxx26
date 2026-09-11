//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest -Wno-unused-variable

// Upstream bloomberg/clang-p2996#184: a function-template NTTP of
// consteval-only reflection type was spuriously diagnosed when named from the
// local templated context introduced by an expansion statement.

#include <meta>
#include <string_view>

void runtime_consume(std::string_view) {}

template <std::meta::info F>
void invoke() {
  template for (constexpr auto P : std::define_static_array(std::meta::parameters_of(F)))
    runtime_consume(std::meta::identifier_of(F));
}

int main(int, char**) {
  invoke<^^runtime_consume>();
  return 0;
}
