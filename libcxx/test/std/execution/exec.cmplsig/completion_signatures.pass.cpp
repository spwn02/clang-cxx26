//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <execution>

// template<completion-signature... Fns>
//   struct completion_signatures;
// template<class Sndr, class Env = env<>, ...>
//     requires sender_in<Sndr, Env>
//   using value_types_of_t = ...;
// template<class Sndr, class Env = env<>, ...>
//     requires sender_in<Sndr, Env>
//   using error_types_of_t = ...;
// template<class Sndr, class Env = env<>>
//     requires sender_in<Sndr, Env>
//   constexpr bool sends_stopped = ...;

#include <execution>
#include <tuple>
#include <type_traits>
#include <variant>

using namespace std::execution;

using CS = completion_signatures<set_value_t(), set_value_t(int, float), set_error_t(int), set_stopped_t()>;

struct MySender {
  using sender_concept = sender_tag;
  auto get_env() const noexcept { return env<>{}; }
  template <class Self>
  static consteval auto get_completion_signatures() {
    return CS{};
  }
};

static_assert(std::is_same_v<value_types_of_t<MySender>, std::variant<std::tuple<>, std::tuple<int, float>>>);
static_assert(std::is_same_v<error_types_of_t<MySender>, std::variant<int>>);
static_assert(sends_stopped<MySender>);

using CSNoStopped = completion_signatures<set_value_t()>;
struct NoStoppedSender {
  using sender_concept = sender_tag;
  auto get_env() const noexcept { return env<>{}; }
  template <class Self>
  static consteval auto get_completion_signatures() {
    return CSNoStopped{};
  }
};
static_assert(!sends_stopped<NoStoppedSender>);

using CSNoValues = completion_signatures<set_error_t(int)>;
struct NoValuesSender {
  using sender_concept = sender_tag;
  auto get_env() const noexcept { return env<>{}; }
  template <class Self>
  static consteval auto get_completion_signatures() {
    return CSNoValues{};
  }
};
static_assert(std::is_same_v<value_types_of_t<NoValuesSender>, __empty_variant>);

int main(int, char**) { return 0; }
