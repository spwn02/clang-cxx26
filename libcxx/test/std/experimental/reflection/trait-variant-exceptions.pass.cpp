//===----------------------------------------------------------------------===//
//
// Copyright 2026 Bloomberg Finance L.P.
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

// <meta>

#include <meta>
#include <variant>

using namespace std::meta;
struct incomplete_variant;

consteval bool test_variant_traits() {
  if (variant_size(^^std::variant<int, bool>) != 2 ||
      variant_alternative(1, ^^std::variant<int, bool>) != ^^bool)
    return false;
  try { (void)variant_size(^^incomplete_variant); } catch (exception const&) {}
  try { (void)variant_alternative(2, ^^std::variant<int, bool>); } catch (exception const&) { return true; }
  return false;
}

static_assert(test_variant_traits());

int main() {}
