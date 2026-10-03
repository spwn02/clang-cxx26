//===----------------------------------------------------------------------===//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20 || c++23
// ADDITIONAL_COMPILE_FLAGS: -freflection

#include <meta>
using namespace std::meta;

// P4033R1 define_enum Returns: targetEnum.
enum class K;
enum class Empty;
consteval {
  if (define_enum(^^K, {enumerator_spec({.name="A"})}) != ^^K)
    throw 0;
  if (define_enum(^^Empty, {}) != ^^Empty)
    throw 0;
}
static_assert(K::A == static_cast<K>(0));
static_assert(enumerators_of(^^Empty).empty());
int main(int, char**) {}
