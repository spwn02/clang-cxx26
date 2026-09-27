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
#include <tuple>

using namespace std::meta;
struct incomplete_tuple;

consteval bool test_tuple_traits() {
  if (tuple_size(^^std::tuple<int, bool>) != 2 ||
      tuple_element(1, ^^std::tuple<int, bool>) != ^^bool)
    return false;
  try { (void)tuple_size(^^incomplete_tuple); } catch (exception const&) {}
  try { (void)tuple_element(2, ^^std::tuple<int, bool>); } catch (exception const&) { return true; }
  return false;
}

static_assert(test_tuple_traits());

int main() {}
