//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <execution>

// [exec.snd.transform]: transform_sender(sndr, env) applies the transformations of the domains in turn and has the
// exception specification of the whole expression. [exec.snd.apply]: apply_sender(dom, tag, sndr, args...) applies
// dom.apply_sender if that is well-formed and default_domain otherwise, is constrained on the result being
// well-formed and has its exception specification.

#include <cassert>
#include <concepts>
#include <execution>
#include <type_traits>
#include <utility>

namespace ex = std::execution;

// A domain whose transformation is the identity, potentially throwing or not.
struct ThrowingDomain {
  template <class Tag, class Sndr, class Env>
  static constexpr auto transform_sender(Tag, Sndr&& sndr, const Env&) {
    return std::forward<Sndr>(sndr);
  }
};
struct NothrowDomain {
  template <class Tag, class Sndr, class Env>
  static constexpr auto transform_sender(Tag, Sndr&& sndr, const Env&) noexcept {
    return std::forward<Sndr>(sndr);
  }
};
template <class D>
struct DomainEnv {
  D query(ex::get_domain_t) const noexcept { return {}; }
};

// the exception specification of the domain's transformation shows up in transform_sender
static_assert(!noexcept(ex::transform_sender(ex::just(1), DomainEnv<ThrowingDomain>{})));
static_assert(noexcept(ex::transform_sender(ex::just(1), DomainEnv<NothrowDomain>{})));
static_assert(noexcept(ex::transform_sender(ex::just(1), ex::env<>{})));
static_assert(std::same_as<decltype(ex::transform_sender(ex::just(1), DomainEnv<ThrowingDomain>{})),
                           decltype(ex::just(1))>);

// a domain that rewrites the sender: transform_sender continues with the domain of the new sender
struct Rewritten {
  using sender_concept = ex::sender_tag;
  ex::env<> get_env() const noexcept { return {}; }
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t()>{};
  }
};
struct RewriteDomain {
  template <class Tag, class Sndr, class Env>
    requires(!std::same_as<std::remove_cvref_t<Sndr>, Rewritten>)
  static constexpr Rewritten transform_sender(Tag, Sndr&&, const Env&) noexcept {
    return {};
  }
};
static_assert(std::same_as<decltype(ex::transform_sender(ex::just(1), DomainEnv<RewriteDomain>{})), Rewritten>);
static_assert(noexcept(ex::transform_sender(ex::just(1), DomainEnv<RewriteDomain>{})));

struct ThrowingRewriteDomain {
  template <class Tag, class Sndr, class Env>
    requires(!std::same_as<std::remove_cvref_t<Sndr>, Rewritten>)
  static constexpr Rewritten transform_sender(Tag, Sndr&&, const Env&) {
    return {};
  }
};
static_assert(!noexcept(ex::transform_sender(ex::just(1), DomainEnv<ThrowingRewriteDomain>{})));

// apply_sender: the domain's own application, the default domain's otherwise
struct ApplyTag {
  template <class Sndr>
  constexpr int apply_sender(Sndr&&) const noexcept {
    return 1;
  }
};
struct ApplyDomain {
  template <class Tag, class Sndr>
  static constexpr int apply_sender(Tag, Sndr&&) noexcept {
    return 2;
  }
};
struct ThrowingApplyDomain {
  template <class Tag, class Sndr>
  static constexpr int apply_sender(Tag, Sndr&&) {
    return 3;
  }
};
static_assert(ex::apply_sender(ApplyDomain{}, ApplyTag{}, ex::just()) == 2);
static_assert(ex::apply_sender(ex::default_domain{}, ApplyTag{}, ex::just()) == 1);
static_assert(ex::apply_sender(NothrowDomain{}, ApplyTag{}, ex::just()) == 1); // a domain without apply_sender
static_assert(noexcept(ex::apply_sender(ApplyDomain{}, ApplyTag{}, ex::just())));
static_assert(!noexcept(ex::apply_sender(ThrowingApplyDomain{}, ApplyTag{}, ex::just())));
static_assert(noexcept(ex::apply_sender(ex::default_domain{}, ApplyTag{}, ex::just())));

// constrained: an unusable combination is not a hard error
struct NoApplyTag {};
template <class Domain, class Tag>
concept can_apply_sender = requires(Domain d, Tag t) { ex::apply_sender(d, t, ex::just()); };
static_assert(can_apply_sender<ex::default_domain, ApplyTag>);
static_assert(can_apply_sender<ApplyDomain, NoApplyTag>);
static_assert(!can_apply_sender<ex::default_domain, NoApplyTag>);
static_assert(!can_apply_sender<NothrowDomain, NoApplyTag>);

int main(int, char**) {
  assert(ex::apply_sender(ApplyDomain{}, ApplyTag{}, ex::just()) == 2);
  return 0;
}
