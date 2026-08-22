//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: no-threads

// <execution>

// namespace execution {
//   struct when_all_with_variant_t { ... };
//   inline constexpr when_all_with_variant_t when_all_with_variant{};
// }

#include <cassert>
#include <execution>
#include <type_traits>
#include <variant>

using namespace std::execution;

int main(int, char**) {
  // [exec.when.all]p18/p19: when_all_with_variant(sndrs...) is expression-equivalent to
  // when_all(into_variant(sndrs)...) -- each child's own value datums (whatever they are) get
  // wrapped in a variant<tuple<...>> by into_variant first, then when_all concatenates those
  // per-child variants into its own value completion.
  {
    auto r = std::this_thread::sync_wait(when_all_with_variant(just(1, 2), just(3.5)));
    assert(r.has_value());
    using __v1 = std::variant<std::tuple<int, int>>;
    using __v2 = std::variant<std::tuple<double>>;
    assert(*r == std::tuple(__v1(std::tuple(1, 2)), __v2(std::tuple(3.5))));
    static_assert(std::is_same_v<completion_signatures_of_t<decltype(when_all_with_variant(just(1, 2), just(3.5))), env<>>,
                                  completion_signatures<set_value_t(__v1, __v2)>>);
  }
  return 0;
}
