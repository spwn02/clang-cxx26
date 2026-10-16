//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <execution>

// struct schedule_t { ... };
// inline constexpr schedule_t schedule{};

#include <cassert>
#include <execution>
#include <type_traits>

using namespace std::execution;

struct MySender {
  using sender_concept = sender_tag;
  int tag = 42;
  auto get_env() const noexcept { return env<>{}; }
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return completion_signatures<set_value_t()>{};
  }
};

struct MyScheduler {
  using scheduler_concept = scheduler_tag;
  MySender schedule() const noexcept { return MySender{}; }
  forward_progress_guarantee query(get_forward_progress_guarantee_t) const noexcept {
    return forward_progress_guarantee::parallel;
  }
  bool operator==(const MyScheduler&) const = default;
};
static_assert(scheduler<MyScheduler>);

static_assert(noexcept(schedule(MyScheduler{})));
static_assert(std::is_same_v<decltype(schedule(MyScheduler{})), MySender>);

int main(int, char**) {
  assert(schedule(MyScheduler{}).tag == 42);
  return 0;
}
