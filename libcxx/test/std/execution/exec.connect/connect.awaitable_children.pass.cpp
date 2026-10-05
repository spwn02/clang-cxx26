//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: libcpp-has-no-incomplete-pstl

// <execution>

// The operation state of connecting an awaitable (operation-state-task, [exec.connect]) cannot be moved
// ([exec.opstate.general] forbids moving the operation state of a library-provided sender, and the library
// does not): every adaptor of the library has to build the operation state of an awaitable child in place.

#include <execution>
#include <stop_token>
#include <coroutine>
#include <cassert>
#include <thread>
#include <optional>
#include <cstdio>
namespace ex=std::execution;
struct Awaiter { bool await_ready() noexcept{return true;} void await_suspend(std::coroutine_handle<>) noexcept{} int await_resume(){return 42;} };
struct R { using receiver_concept=ex::receiver_tag; int* p=nullptr; auto get_env() const noexcept { return ex::env(ex::prop(ex::get_start_scheduler, ex::inline_scheduler{}), ex::prop(ex::get_scheduler, ex::inline_scheduler{})); } template<class...T> void set_value(T&&...) && noexcept {if(p)++*p;} template<class T> void set_error(T&&) && noexcept {if(p)*p+=10;} void set_stopped() && noexcept {if(p)*p+=100;} };
template <class S> int run(S&& s){ int n=0; auto op=ex::connect(std::forward<S>(s), R{&n}); ex::start(op); return n; }
int main(int, char**) {
#define T(name, expr) { int r = run(expr); assert(r == 1); (void)name; }
  T("direct", Awaiter{});
  T("then", ex::then(Awaiter{}, [](int){}));
  T("let_value", ex::let_value(ex::just(), []{ return Awaiter{}; }));
  T("let_value2", ex::let_value(Awaiter{}, [](int){ return ex::just(); }));
  T("when_all", ex::when_all(Awaiter{}, ex::just()));
  T("continues_on", ex::continues_on(Awaiter{}, ex::inline_scheduler{}));
  T("starts_on", ex::starts_on(ex::inline_scheduler{}, Awaiter{}));
  T("on", ex::on(ex::inline_scheduler{}, Awaiter{}));
  T("into_variant", ex::into_variant(Awaiter{}));
  T("stopped_as_optional", ex::stopped_as_optional(Awaiter{}));
  T("stopped_as_error", ex::stopped_as_error(Awaiter{}, 1));
  T("write_env", ex::write_env(Awaiter{}, ex::env<>{}));
  T("unstoppable", ex::unstoppable(Awaiter{}));
  T("bulk", ex::bulk(Awaiter{}, std::execution::seq, 2, [](int, int){}));
  T("affine_on", ex::affine_on(Awaiter{}, ex::inline_scheduler{}));
  { auto v = std::this_thread::sync_wait(Awaiter{}); assert(v && std::get<0>(*v) == 42); }
  { ex::counting_scope sc; auto f = ex::spawn_future(Awaiter{}, sc.get_token()); auto v = std::this_thread::sync_wait(std::move(f)); std::this_thread::sync_wait(sc.join()); assert(v && std::get<0>(*v) == 42); }
  { ex::counting_scope sc; auto s = ex::associate(Awaiter{}, sc.get_token()); T("associate", std::move(s)); std::this_thread::sync_wait(sc.join()); }
  return 0;
}
