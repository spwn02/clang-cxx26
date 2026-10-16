//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <execution>

// [exec.domain.indeterminate]: indeterminate_domain<Domains...> and its common_type rules, [exec.snd.expos]
// COMMON-DOMAIN.

#include <cassert>
#include <concepts>
#include <execution>
#include <type_traits>

namespace ex = std::execution;

struct A {};
struct B {};
struct C {};
using Ind = ex::indeterminate_domain<>;

static_assert(std::is_class_v<ex::indeterminate_domain<>>);
static_assert(std::is_class_v<ex::indeterminate_domain<A, B>>);
static_assert(std::is_default_constructible_v<ex::indeterminate_domain<A>>);
// constructible from anything, noexcept
static_assert(std::is_nothrow_constructible_v<ex::indeterminate_domain<A>, A>);
static_assert(std::is_nothrow_constructible_v<ex::indeterminate_domain<A>, int>);
static_assert(std::is_nothrow_constructible_v<ex::indeterminate_domain<A, B>, ex::indeterminate_domain<C>>);

// common_type of two indeterminate domains: the union of the domains, without duplicates, in order of appearance
static_assert(std::same_as<std::common_type_t<ex::indeterminate_domain<A>, ex::indeterminate_domain<B>>,
                           ex::indeterminate_domain<A, B>>);
static_assert(std::same_as<std::common_type_t<ex::indeterminate_domain<A, B>, ex::indeterminate_domain<B, C>>,
                           ex::indeterminate_domain<A, B, C>>);
static_assert(std::same_as<std::common_type_t<ex::indeterminate_domain<A>, ex::indeterminate_domain<A>>,
                           ex::indeterminate_domain<A>>);
static_assert(std::same_as<std::common_type_t<Ind, Ind>, Ind>);
static_assert(std::same_as<std::common_type_t<ex::indeterminate_domain<A, A, B>, Ind>, ex::indeterminate_domain<A, B>>);

// with a domain that is not an indeterminate domain: that domain if there are no domains, an indeterminate domain of
// the domains and that one otherwise (in both argument orders)
static_assert(std::same_as<std::common_type_t<Ind, A>, A>);
static_assert(std::same_as<std::common_type_t<A, Ind>, A>);
static_assert(std::same_as<std::common_type_t<ex::indeterminate_domain<A>, B>, ex::indeterminate_domain<A, B>>);
static_assert(std::same_as<std::common_type_t<B, ex::indeterminate_domain<A>>, ex::indeterminate_domain<A, B>>);
static_assert(std::same_as<std::common_type_t<ex::indeterminate_domain<A, B>, A>, ex::indeterminate_domain<A, B>>);

// Without an indeterminate domain there is no common type of two different domains.
template <class T, class U>
concept has_common_type = requires { typename std::common_type_t<T, U>; };
static_assert(!has_common_type<A, B>);
static_assert(has_common_type<A, A>);

// transform_sender: the result of the default domain
struct Sndr {
  using sender_concept = ex::sender_tag;
};
template <class Dom>
using transformed_t = decltype(Dom::transform_sender(ex::start, std::declval<Sndr>(), ex::env<>{}));
static_assert(std::same_as<transformed_t<ex::indeterminate_domain<A, B>>, transformed_t<ex::default_domain>>);
static_assert(noexcept(ex::indeterminate_domain<A>::transform_sender(ex::start, std::declval<Sndr>(), ex::env<>{})));

// Mandates: every domain of the indeterminate domain either does not transform the sender or yields the same type
struct SameDomain {
  template <class Tag, class S, class Env>
  static constexpr S transform_sender(Tag, S&& s, const Env&) noexcept {
    return std::forward<S>(s);
  }
};
struct SameDomainForAnotherSender {
  template <class Tag, class Env>
  static constexpr int transform_sender(Tag, int, const Env&) noexcept {
    return 0;
  }
};
static_assert(std::same_as<transformed_t<ex::indeterminate_domain<SameDomain, SameDomainForAnotherSender>>, Sndr>);

// COMMON-DOMAIN through the attributes of a sender: see exec.then/then.attributes.pass.cpp.

int main(int, char**) { return 0; }
