//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <execution>

// template<class Sndr, class... Env>
//   consteval auto get_completion_signatures() -> valid-completion-signatures auto;
// template<class Sndr, class... Env>
//     requires sender_in<Sndr, Env...>
//   using completion_signatures_of_t = decltype(get_completion_signatures<Sndr, Env...>());
// template<class Sndr, class... Env>
//   concept sender_in = ...;

#include <execution>
#include <type_traits>

using namespace std::execution;

using CS = completion_signatures<set_value_t(int), set_error_t(int), set_stopped_t()>;

struct MySender {
  using sender_concept = sender_tag;
  auto get_env() const noexcept { return env<>{}; }
  template <class Self>
  static consteval auto get_completion_signatures() {
    return CS{};
  }
};
static_assert(sender_in<MySender>);
static_assert(std::is_same_v<completion_signatures_of_t<MySender>, CS>);
// sender_in<Sndr, Env> (1-Env form, matching value_types_of_t's default) also works, since
// it routes through transform_sender -- see exec.snd.transform/transform_sender.pass.cpp.
static_assert(sender_in<MySender, env<>>);
static_assert(std::is_same_v<completion_signatures_of_t<MySender, env<>>, CS>);

struct NoCompletionSigs {
  using sender_concept = sender_tag;
  auto get_env() const noexcept { return env<>{}; }
};
static_assert(sender<NoCompletionSigs>);
static_assert(!sender_in<NoCompletionSigs>);

int main(int, char**) { return 0; }
