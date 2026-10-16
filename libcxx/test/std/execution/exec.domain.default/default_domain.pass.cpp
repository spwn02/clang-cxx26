//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <execution>

// struct default_domain { ... };
// struct get_domain_t { ... };
// inline constexpr get_domain_t get_domain{};

#include <execution>
#include <type_traits>

using namespace std;
using namespace std::execution;

struct MySender {
  using sender_concept = sender_tag;
  auto get_env() const noexcept { return env<>{}; }
};

// default_domain::transform_sender falls back to returning the sender unchanged, since no
// sender in this fork's current scope defines a per-tag `.transform_sender` override (see
// the deferral comment on default_domain::transform_sender in __execution/domain.h).
static_assert(std::is_same_v<decltype(default_domain::transform_sender(set_value_t{}, MySender{}, env<>{})), MySender>);

// get_domain falls back to default_domain() when nothing in the env answers the query.
static_assert(std::is_same_v<decltype(get_domain(env<>{})), default_domain>);
static_assert(forwarding_query(get_domain));

int main(int, char**) { return 0; }
