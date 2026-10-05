//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <execution>

// [execution.syn], [exec.general]: sender_adaptor_closure<D> and with_awaitable_senders<Promise> take a class-type
// template argument (decays-to<T, T> && is_class_v<T>): not a non-class type, a reference or a cv-qualified type.
// [exec.parallel.scheduler.replacement]: receiver_proxy::try_query<P, class-type Query>.

#include <execution>
#include <stop_token>
#include <type_traits>

namespace ex = std::execution;

struct Closure {};
struct Promise {};

template <class T>
concept can_name_closure = requires { typename ex::sender_adaptor_closure<T>; };
template <class T>
concept can_name_with_awaitable_senders = requires { typename ex::with_awaitable_senders<T>; };

static_assert(can_name_closure<Closure>);
static_assert(!can_name_closure<int>);
static_assert(!can_name_closure<Closure&>);
static_assert(!can_name_closure<Closure&&>);
static_assert(!can_name_closure<const Closure>);
static_assert(!can_name_closure<Closure*>);
static_assert(!can_name_closure<int[2]>);

static_assert(can_name_with_awaitable_senders<Promise>);
static_assert(!can_name_with_awaitable_senders<int>);
static_assert(!can_name_with_awaitable_senders<Promise&>);
static_assert(!can_name_with_awaitable_senders<const Promise>);
static_assert(!can_name_with_awaitable_senders<void>);

namespace repl = ex::parallel_scheduler_replacement;
template <class P, class Q>
concept can_try_query = requires(const repl::receiver_proxy& r, Q q) { r.template try_query<P, Q>(static_cast<std::remove_cvref_t<Q>>(q)); };
static_assert(can_try_query<std::inplace_stop_token, std::get_stop_token_t>);
static_assert(!can_try_query<std::inplace_stop_token, int>);
static_assert(!can_try_query<std::inplace_stop_token, std::get_stop_token_t&>);

int main(int, char**) { return 0; }
