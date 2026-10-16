//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <execution>

// template<class Domain, class Tag, sender Sndr, class... Args>
//   constexpr decltype(auto) apply_sender(Domain dom, Tag, Sndr&& sndr, Args&&... args) noexcept(see below);

#include <cassert>
#include <execution>

using namespace std::execution;

struct MySender {
  using sender_concept = sender_tag;
  auto get_env() const noexcept { return env<>{}; }
};

struct MyTag {
  int apply_sender(MySender&&, int v) const { return v + 1; }
};

int main(int, char**) {
  // default_domain::apply_sender forwards to Tag().apply_sender(sndr, args...).
  assert(apply_sender(default_domain{}, MyTag{}, MySender{}, 41) == 42);
  return 0;
}
