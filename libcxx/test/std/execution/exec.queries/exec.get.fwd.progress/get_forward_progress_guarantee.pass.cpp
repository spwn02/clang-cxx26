//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <execution>

// enum class forward_progress_guarantee { concurrent, parallel, weakly_parallel };
// struct get_forward_progress_guarantee_t { ... };
// inline constexpr get_forward_progress_guarantee_t get_forward_progress_guarantee{};

#include <cassert>
#include <execution>
#include <type_traits>

using namespace std::execution;

static_assert(std::is_enum_v<forward_progress_guarantee>);
static_assert(forward_progress_guarantee::concurrent != forward_progress_guarantee::parallel);
static_assert(forward_progress_guarantee::parallel != forward_progress_guarantee::weakly_parallel);

struct HasGuarantee {
  constexpr forward_progress_guarantee query(get_forward_progress_guarantee_t) const noexcept {
    return forward_progress_guarantee::parallel;
  }
};
static_assert(noexcept(get_forward_progress_guarantee(HasGuarantee{})));
static_assert(std::is_same_v<decltype(get_forward_progress_guarantee(HasGuarantee{})), forward_progress_guarantee>);

struct NoGuarantee {};

// Expressed via a named concept, not an inline `requires(...) {...}` passed directly to
// static_assert: on this fork, the latter hard-errors instead of evaluating false when the sole
// operator() candidate's constraints aren't satisfied (reproduced in isolation, unrelated to
// <__execution/get_forward_progress_guarantee.h> itself -- a named concept sidesteps it, and is
// the same pattern <__execution/scheduler.h>'s `scheduler` concept already relies on).
template <class T>
concept __has_get_forward_progress_guarantee = requires(T t) { get_forward_progress_guarantee(t); };
static_assert(!__has_get_forward_progress_guarantee<NoGuarantee>);

int main(int, char**) {
  assert(get_forward_progress_guarantee(HasGuarantee{}) == forward_progress_guarantee::parallel);
  return 0;
}
