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
// RUN:     %s -o %t/std-execution-scopes.sh.cpp.tsk
// RUN: %{exec} %t/std-execution-scopes.sh.cpp.tsk

// [execution.syn]: "template<class Token> concept scope_association = see below;"
// "namespace parallel_scheduler_replacement { struct receiver_proxy;
// struct bulk_item_receiver_proxy; struct parallel_scheduler_backend;
// shared_ptr<parallel_scheduler_backend> query_parallel_scheduler_backend(); }"
import std;
namespace ex = std::execution;
namespace repl = ex::parallel_scheduler_replacement;
using assoc = decltype(std::declval<ex::simple_counting_scope::token>().try_associate());
static_assert(ex::scope_association<assoc>);
static_assert(ex::scope_token<ex::simple_counting_scope::token>);
static_assert(ex::scope_token<ex::counting_scope::token>);
static_assert(ex::simple_counting_scope::max_associations > 0);
static_assert(ex::counting_scope::max_associations > 0);
static_assert(std::is_base_of_v<repl::receiver_proxy, repl::bulk_item_receiver_proxy>);
static_assert(std::is_abstract_v<repl::parallel_scheduler_backend>);
static_assert(std::same_as<decltype(repl::query_parallel_scheduler_backend()),
                          std::shared_ptr<repl::parallel_scheduler_backend>>);
int main(int, char**) {
  ex::simple_counting_scope scope;
  { auto a = scope.get_token().try_associate(); if (!a) return 1; }
  return !std::this_thread::sync_wait(scope.join());
}
