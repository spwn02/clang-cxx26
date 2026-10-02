// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20 || c++23
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

#include <meta>
using namespace std::meta;

static_assert(symbol_of(operators::op_co_await) == "co_await");
static_assert(u8symbol_of(operators::op_co_await) == u8"co_await");

int main() {}
