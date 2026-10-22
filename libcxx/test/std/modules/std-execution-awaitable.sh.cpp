//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: clang-modules-build
// UNSUPPORTED: gcc

// XFAIL: has-no-cxx-module-support


// RUN: mkdir %t
// RUN: %{cxx} %{compile_flags} -std=c++26 \
// RUN:     -Wno-reserved-module-identifier -Wno-reserved-user-defined-literal \
// RUN:     --precompile -o %t/std.pcm -c %{module-dir}/std.cppm
// RUN: %{cxx} %{compile_flags} %{link_flags} -std=c++26 \
// RUN:     -fmodule-file=std=%t/std.pcm %t/std.pcm \
// RUN:     %s -o %t/std-execution-awaitable.sh.cpp.tsk
// RUN: %{exec} %t/std-execution-awaitable.sh.cpp.tsk

// [exec.awaitable]: GET-AWAITER is resolved in the importing translation unit by the compiler (an operator co_await
// chosen by overload resolution, a final promise's await_transform); the dependent builtin call is part of the module.
import std;
namespace ex = std::execution;
namespace Lib {
struct Awaiter {
  bool await_ready() noexcept { return true; }
  void await_suspend(std::coroutine_handle<>) noexcept {}
  int await_resume() & { return 41; }
};
struct Rvalue { Awaiter operator co_await() && { return {}; } };
struct Adl {};
Awaiter operator co_await(Adl) { return {}; }
} // namespace Lib
static_assert(ex::sender<Lib::Awaiter>);
static_assert(ex::sender<Lib::Rvalue>);
static_assert(ex::sender<Lib::Adl>);
static_assert(ex::sender_in<Lib::Adl, ex::env<>>);
int main(int, char**) {
  auto r = std::this_thread::sync_wait(Lib::Rvalue{});
  return (r && std::get<0>(*r) == 41) ? 0 : 1;
}
