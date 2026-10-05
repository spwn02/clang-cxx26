//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <execution>

// [exec.domain.default]: default_domain::transform_sender(Tag, sndr, env) is
//   tag_of_t<Sndr>().transform_sender(Tag(), forward<Sndr>(sndr), env) if that is well-formed, otherwise
//   static_cast<Sndr>(forward<Sndr>(sndr)); its exception specification is that of the expression.
// default_domain::apply_sender(Tag, sndr, args...) is Tag().apply_sender(forward<Sndr>(sndr), args...), constrained on
// that expression being well-formed, with its exception specification.

#include <cassert>
#include <concepts>
#include <execution>
#include <type_traits>
#include <utility>

namespace ex = std::execution;

// tag_of_t<Sndr> is the type of the first element of the structured binding `auto&& [tag, data, ...children] = sndr`.
struct Transforming {
  template <class Sndr, class Env>
  constexpr auto transform_sender(ex::set_value_t, Sndr&&, const Env&) const noexcept {
    return ex::just(99);
  }
};
struct TaggedSender {
  using sender_concept = ex::sender_tag;
  Transforming tag;
  int data;
};
static_assert(std::same_as<ex::tag_of_t<TaggedSender>, Transforming>);
static_assert(std::same_as<ex::tag_of_t<const TaggedSender&>, Transforming>);

// the tag of the sender provides the transformation for the tag it is asked for
static_assert(std::same_as<decltype(ex::default_domain::transform_sender(ex::set_value, TaggedSender{}, ex::env<>{})),
                           decltype(ex::just(99))>);
static_assert(noexcept(ex::default_domain::transform_sender(ex::set_value, TaggedSender{}, ex::env<>{})));
// ... and not for another one: the sender is returned as it is
static_assert(std::same_as<decltype(ex::default_domain::transform_sender(ex::start, TaggedSender{}, ex::env<>{})),
                           TaggedSender>);

// A sender that cannot be decomposed has no tag_of_t, and that is not an error.
struct OneMember {
  using sender_concept = ex::sender_tag;
  int data;
};
struct PrivateMembers {
  using sender_concept = ex::sender_tag;

  [[maybe_unused]] int pub = 0;

private:
  [[maybe_unused]] int a = 0;
  [[maybe_unused]] int b = 0;
};
struct NoMembers {
  using sender_concept = ex::sender_tag;
};
template <class S>
concept has_tag = requires { typename ex::tag_of_t<S>; };
static_assert(!has_tag<OneMember>);
static_assert(!has_tag<PrivateMembers>);
static_assert(!has_tag<NoMembers>);
static_assert(!has_tag<int>);
static_assert(has_tag<TaggedSender>);
static_assert(std::same_as<decltype(ex::default_domain::transform_sender(ex::set_value, OneMember{}, ex::env<>{})),
                           OneMember>);
static_assert(std::same_as<decltype(ex::default_domain::transform_sender(ex::set_value, NoMembers{}, ex::env<>{})),
                           NoMembers>);

// the exception specification is the one of the expression: returning an rvalue sender moves it
struct ThrowingMove {
  using sender_concept = ex::sender_tag;
  ThrowingMove() = default;
  ThrowingMove(const ThrowingMove&) noexcept(false) {}
  ThrowingMove(ThrowingMove&&) noexcept(false) {}
};
static_assert(!noexcept(ex::default_domain::transform_sender(ex::start, std::declval<ThrowingMove>(), ex::env<>{})));
// an lvalue is returned as a reference
static_assert(noexcept(ex::default_domain::transform_sender(ex::start, std::declval<ThrowingMove&>(), ex::env<>{})));
static_assert(std::same_as<decltype(ex::default_domain::transform_sender(ex::start, std::declval<ThrowingMove&>(),
                                                                          ex::env<>{})),
                           ThrowingMove&>);
struct NothrowMove {
  using sender_concept = ex::sender_tag;
};
static_assert(noexcept(ex::default_domain::transform_sender(ex::start, std::declval<NothrowMove>(), ex::env<>{})));

// a tag whose transformation is potentially throwing
struct ThrowingTag {
  template <class Sndr, class Env>
  constexpr auto transform_sender(ex::set_value_t, Sndr&&, const Env&) const {
    return ex::just(1);
  }
};
struct ThrowingTagSender {
  using sender_concept = ex::sender_tag;
  ThrowingTag tag;
  int data;
};
static_assert(!noexcept(ex::default_domain::transform_sender(ex::set_value, ThrowingTagSender{}, ex::env<>{})));

// apply_sender: the tag applies the sender
struct ApplyTag {
  template <class Sndr>
  constexpr int apply_sender(Sndr&&) const noexcept {
    return 42;
  }
  template <class Sndr>
  constexpr int apply_sender(Sndr&&, int x) const {
    return x;
  }
};
static_assert(ex::default_domain::apply_sender(ApplyTag{}, ex::just()) == 42);
static_assert(noexcept(ex::default_domain::apply_sender(ApplyTag{}, ex::just())));
static_assert(ex::default_domain::apply_sender(ApplyTag{}, ex::just(), 7) == 7);
static_assert(!noexcept(ex::default_domain::apply_sender(ApplyTag{}, ex::just(), 7)));

// constrained: no hard error for a tag that cannot apply
struct NoApplyTag {};
template <class Tag, class... Args>
concept can_apply = requires(Tag tag, Args... args) { ex::default_domain::apply_sender(tag, ex::just(), args...); };
static_assert(can_apply<ApplyTag>);
static_assert(can_apply<ApplyTag, int>);
static_assert(!can_apply<ApplyTag, int, int>);
static_assert(!can_apply<NoApplyTag>);
static_assert(!can_apply<int>);

int main(int, char**) { return 0; }
