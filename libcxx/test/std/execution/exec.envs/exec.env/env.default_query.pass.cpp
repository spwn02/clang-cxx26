//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <execution>

#include <execution>
struct query { query() = delete; explicit query(int) {} };
struct environment { int query(::query) const { return 1; } };
template <class E> concept accepts_query = requires(const E& e) { e.query(query(0)); };
// [exec.env]: "env.query(QueryTag(), std::forward<Args>(args)...);"
static_assert(!accepts_query<std::execution::env<environment>>);
int main(int, char**) { return 0; }
