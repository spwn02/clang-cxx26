//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <execution>
// [execution.syn]
// struct get_scheduler_allocator_t;
// inline constexpr get_scheduler_allocator_t get_scheduler_allocator{};

#include <cassert>
#include <execution>
#include <memory>
#include <type_traits>

struct EnvWithAllocator {
  std::allocator<int> query(std::get_allocator_t) const noexcept { return {}; }
};

struct EnvWithSchedulerAllocator {
  std::allocator<long> query(std::get_scheduler_allocator_t) const noexcept { return {}; }
  std::allocator<int> query(std::get_allocator_t) const noexcept { return {}; }
};

struct NoAllocator {};

template <class Env>
concept CanGetSchedulerAllocator = requires(const Env& env) { std::get_scheduler_allocator(env); };

int main(int, char**) {
  using namespace std;

  static_assert(forwarding_query(get_scheduler_allocator));
  static_assert(is_same_v<decltype(get_scheduler_allocator(declval<const EnvWithAllocator&>())), allocator<int>>);
  static_assert(
      is_same_v<decltype(get_scheduler_allocator(declval<const EnvWithSchedulerAllocator&>())), allocator<long>>);
  static_assert(noexcept(get_scheduler_allocator(declval<const EnvWithAllocator&>())));
  static_assert(noexcept(get_scheduler_allocator(declval<const EnvWithSchedulerAllocator&>())));
  static_assert(CanGetSchedulerAllocator<EnvWithAllocator>);
  static_assert(CanGetSchedulerAllocator<EnvWithSchedulerAllocator>);
  static_assert(!CanGetSchedulerAllocator<NoAllocator>);

  assert(get_scheduler_allocator(EnvWithAllocator{}) == allocator<int>{});
  assert(get_scheduler_allocator(EnvWithSchedulerAllocator{}) == allocator<long>{});
  return 0;
}
