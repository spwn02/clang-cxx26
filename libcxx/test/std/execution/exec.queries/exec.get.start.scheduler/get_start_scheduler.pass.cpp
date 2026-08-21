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
//   struct get_start_scheduler_t { ... };
//   inline constexpr get_start_scheduler_t get_start_scheduler{};
// }

#include <cassert>
#include <execution>

using namespace std::execution;

struct env_with_sch {
  run_loop* loop;
  auto query(get_start_scheduler_t) const noexcept { return loop->get_scheduler(); }
};

template <class T>
concept has_get_start_scheduler = requires(const T& t) { get_start_scheduler(t); };

int main(int, char**) {
  static_assert(std::forwarding_query(get_start_scheduler));
  static_assert(!has_get_start_scheduler<env<>>);

  run_loop loop;
  env_with_sch env{&loop};
  assert((get_start_scheduler(env) == loop.get_scheduler()));

  loop.finish();
  loop.run();
  return 0;
}
