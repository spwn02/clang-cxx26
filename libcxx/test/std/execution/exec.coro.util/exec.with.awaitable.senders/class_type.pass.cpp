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
struct promise;
union union_promise;
template <class T> concept accepts = requires { typename std::execution::with_awaitable_senders<T>; };
// [exec.with.awaitable.senders]: "template<class-type Promise> struct with_awaitable_senders"
static_assert(accepts<promise>);
static_assert(!accepts<int>);
static_assert(!accepts<void>);
int main(int, char**) { return 0; }
