//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++26
// UNSUPPORTED: libcpp-has-no-incomplete-pstl

// P3179R9 adds the execution-policy overloads of the ranges algorithms in C++26: before that, they must not exist,
// while the sequential overloads keep working.

#include <algorithm>
#include <execution>
#include <vector>

using V = std::vector<int>;

template <class T>
concept policy_copy = requires(T& r) { std::ranges::copy(std::execution::seq, r, r); };
template <class T>
concept policy_for_each = requires(T& r) { std::ranges::for_each(std::execution::seq, r, [](int) {}); };
template <class T>
concept policy_set_union = requires(T& r) { std::ranges::set_union(std::execution::seq, r, r, r); };
template <class T>
concept policy_unique_copy = requires(T& r) { std::ranges::unique_copy(std::execution::seq, r, r); };
template <class T>
concept policy_partial_sort_copy = requires(T& r) { std::ranges::partial_sort_copy(std::execution::seq, r, r); };

static_assert(!policy_copy<V>);
static_assert(!policy_for_each<V>);
static_assert(!policy_set_union<V>);
static_assert(!policy_unique_copy<V>);
static_assert(!policy_partial_sort_copy<V>);

static_assert(requires(V& r) { std::ranges::copy(r, r.begin()); });
static_assert(requires(V& r) { std::ranges::for_each(r, [](int) {}); });
static_assert(requires(V& r) { std::ranges::partial_sort_copy(r, r); });
