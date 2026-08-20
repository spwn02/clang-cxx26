//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <execution>

// template<class Sndr>
//   inline constexpr bool enable_sender = ...;
// template<class Sndr>
//   concept sender = ...;
// template<sender Sndr>
//   using tag_of_t = ...;

#include <execution>
#include <type_traits>

using namespace std::execution;

struct MySender {
  using sender_concept = sender_tag;
  auto get_env() const noexcept { return env<>{}; }
};
static_assert(sender<MySender>);
static_assert(enable_sender<MySender>);

struct NotASender {};
static_assert(!sender<NotASender>);
static_assert(!enable_sender<NotASender>);

// tag_of_t via structured bindings: (tag, data) and (tag, data, child) shapes.
struct MyTag {};
struct TwoMember {
  using sender_concept = sender_tag;
  MyTag tag;
  int data;
  auto get_env() const noexcept { return env<>{}; }
};
static_assert(std::is_same_v<tag_of_t<TwoMember>, MyTag>);

struct ChildTag {};
struct ChildSender {
  using sender_concept = sender_tag;
  auto get_env() const noexcept { return env<>{}; }
};
struct ThreeMember {
  using sender_concept = sender_tag;
  ChildTag tag;
  int data;
  ChildSender child;
  auto get_env() const noexcept { return env<>{}; }
};
static_assert(std::is_same_v<tag_of_t<ThreeMember>, ChildTag>);

int main(int, char**) { return 0; }
