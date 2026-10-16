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
// RUN: %{cxx} %{compile_flags} -std=c++26 -freflection-latest \
// RUN:     -Wno-reserved-module-identifier -Wno-reserved-user-defined-literal \
// RUN:     --precompile -o %t/std.pcm -c %{module-dir}/std.cppm
// RUN: %{cxx} %{compile_flags} %{link_flags} -std=c++26 -freflection-latest \
// RUN:     -fmodule-file=std=%t/std.pcm %t/std.pcm \
// RUN:     %s -o %t/std-execution-task.sh.cpp.tsk
// RUN: %{exec} %t/std-execution-task.sh.cpp.tsk

// [execution.syn]: "using type = remove_cvref_t<E>; type error;"
// [task.class]: "using start_scheduler_type = see below;"
import std;
namespace ex = std::execution;
static_assert(std::same_as<ex::with_error<const int&>::type, int>);
static_assert(std::same_as<ex::task<>::start_scheduler_type, ex::task_scheduler>);
static_assert(std::same_as<ex::task<int>, ex::task<int, ex::env<>>>);
int main(int, char**) { return 0; }
