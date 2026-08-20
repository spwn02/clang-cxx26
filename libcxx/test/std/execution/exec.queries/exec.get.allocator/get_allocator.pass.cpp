//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <execution>

// struct get_allocator_t : forwarding_query_t {
//   template<class Env>
//   constexpr auto operator()(const Env&) const noexcept(...);
// };
// inline constexpr get_allocator_t get_allocator{};

#include <cassert>
#include <execution>
#include <memory>
#include <type_traits>

struct HasAllocator {
  std::allocator<int> query(std::execution::get_allocator_t) const noexcept { return {}; }
};

struct NoAllocator {};

template <class Q, class Env>
concept CanGetAllocator = requires(const Env& e, Q q) { q(e); };

int main(int, char**) {
  using namespace std::execution;

  static_assert(forwarding_query(get_allocator));

  static_assert(CanGetAllocator<get_allocator_t, HasAllocator>);
  static_assert(!CanGetAllocator<get_allocator_t, NoAllocator>);

  HasAllocator env;
  static_assert(std::is_same_v<decltype(get_allocator(env)), std::allocator<int>>);
  assert((get_allocator(env) == std::allocator<int>{}));
  static_assert(noexcept(get_allocator(env)));

  return 0;
}
