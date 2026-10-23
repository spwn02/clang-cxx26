//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <execution>

// template<class V = T> void task::promise_type::return_value(V&& value);
//
// The default template argument lets a braced-init-list be returned.

#include <coroutine>
#include <execution>
#include <string>

std::execution::task<std::string> returns_braced() { co_return {}; }
std::execution::task<std::string> returns_literal() { co_return "abc"; }
