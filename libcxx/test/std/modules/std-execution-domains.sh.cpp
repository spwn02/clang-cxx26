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
// RUN:     %s -o %t/std-execution-domains.sh.cpp.tsk
// RUN: %{exec} %t/std-execution-domains.sh.cpp.tsk

// [execution.syn]: indeterminate_domain, get_completion_domain(_t), default_domain, get_domain(_t)
import std;
namespace ex = std::execution;
struct A {};
struct B {};
static_assert(std::is_class_v<ex::indeterminate_domain<A, B>>);
static_assert(std::same_as<std::common_type_t<ex::indeterminate_domain<A>, ex::indeterminate_domain<B>>,
                           ex::indeterminate_domain<A, B>>);
static_assert(std::same_as<decltype(ex::get_domain(ex::env<>{})), ex::default_domain>);
static_assert(std::same_as<decltype(ex::get_completion_domain<ex::set_value_t>(ex::inline_scheduler{}, ex::env<>{})),
                           ex::default_domain>);
static_assert(std::forwarding_query(ex::get_completion_domain<ex::set_error_t>));
static_assert(std::forwarding_query(ex::get_completion_scheduler<ex::set_stopped_t>));
static_assert(std::is_base_of_v<std::forwarding_query_t, ex::get_completion_domain_t<void>>);
static_assert(std::same_as<ex::get_completion_domain_t<>, ex::get_completion_domain_t<void>>);
static_assert(!ex::dependent_sender<int>);
static_assert(ex::dependent_sender<decltype(ex::read_env(std::get_stop_token))>);
static_assert(!ex::dependent_sender<decltype(ex::just())>);
struct Op;
struct InlinableRcvr {
  using receiver_concept = ex::receiver_tag;
  void set_value() && noexcept {}
  static InlinableRcvr make_receiver_for(Op*) noexcept { return {}; }
};
static_assert(ex::inlinable_receiver<InlinableRcvr, Op>);
static_assert(std::forwarding_query(ex::get_await_completion_adaptor));
static_assert(std::is_class_v<ex::affine_t>);
static_assert(ex::sender<decltype(ex::affine(ex::just(1)))>);
static_assert(std::same_as<ex::tag_of_t<decltype(ex::affine(ex::just(1)))>, ex::affine_t>);
static_assert(std::is_class_v<ex::get_await_completion_adaptor_t>);
int main(int, char**) { return 0; }
