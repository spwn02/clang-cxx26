//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20 || c++23
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

// current_function() and current_class() throw meta::exception when the current
// scope is neither a function nor a class (here: a consteval block at namespace
// scope, which needs the try/catch inside the block itself).
#include <meta>

consteval {
  try { (void)std::meta::current_function(); } catch (const std::meta::exception&) { return; }
  throw 0;
}
consteval {
  try { (void)std::meta::current_class(); } catch (const std::meta::exception&) { return; }
  throw 0;
}

int main(int, char**) {}
