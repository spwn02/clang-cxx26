//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <execution>

// template<sender Sndr, queryable Env>
//   constexpr decltype(auto) transform_sender(Sndr&& sndr, const Env& env) noexcept(see below);

#include <execution>
#include <type_traits>

using namespace std::execution;

struct MySender {
  using sender_concept = sender_tag;
  auto get_env() const noexcept { return env<>{}; }
};

// With no env providing get_domain/get_completion_domain and no sender defining a per-tag
// .transform_sender, transform_sender is the identity: the fixed point in transform-recurse
// is reached on the first iteration.
static_assert(std::is_same_v<decltype(transform_sender(MySender{}, env<>{})), MySender>);
static_assert(noexcept(transform_sender(MySender{}, env<>{})));

int main(int, char**) { return 0; }
