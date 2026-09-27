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

using namespace std::meta;

enum signed_enum : short { value };

consteval bool test_sign_traits() {
  if (make_signed(^^unsigned) != ^^int || make_unsigned(^^int) != ^^unsigned)
    return false;
  try { (void)make_signed(^^bool); } catch (exception const&) { return true; }
  return false;
}

consteval bool test_enum_traits() {
  if (make_signed(^^signed_enum) != ^^short || make_unsigned(^^signed_enum) != ^^unsigned short)
    return false;
  try { (void)make_unsigned(^^float); } catch (exception const&) { return true; }
  return false;
}

static_assert(test_sign_traits());
static_assert(test_enum_traits());

int main() {}
