//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <linalg>
// [linalg.layout.transpose.obs] operator== participates when nested mappings
// have a bool-convertible equality comparison.

#include <concepts>
#include <linalg>
#include <mdspan>

template <class A, class B>
concept Comparable = requires(const A& a, const B& b) {
  { a == b } -> std::convertible_to<bool>;
};

using StaticExtents  = std::extents<std::size_t, 2, 3>;
using StaticMapping  = std::linalg::layout_transpose<std::layout_right>::mapping<StaticExtents>;
using DynamicExtents = std::extents<std::size_t, std::dynamic_extent, 3>;
using DynamicMapping = std::linalg::layout_transpose<std::layout_right>::mapping<DynamicExtents>;

struct NoEqualityLayout {
  template <class E>
  struct mapping {
    using extents_type = E;
  };
};

using NoEqualityMapping = std::linalg::layout_transpose<NoEqualityLayout>::mapping<StaticExtents>;

static_assert(Comparable<StaticMapping, StaticMapping>);
static_assert(Comparable<StaticMapping, DynamicMapping>);
static_assert(!Comparable<NoEqualityMapping, NoEqualityMapping>);

constexpr std::layout_right::mapping<std::extents<std::size_t, 3, 2>> nested;
constexpr StaticMapping lhs(nested);
constexpr StaticMapping rhs(nested);
static_assert(lhs == rhs);

constexpr std::layout_right::mapping<std::extents<std::size_t, 3, std::dynamic_extent>>
    dynamic_nested(std::extents<std::size_t, 3, std::dynamic_extent>(2));
constexpr DynamicMapping dynamic_mapping(dynamic_nested);
static_assert(lhs == dynamic_mapping && dynamic_mapping == lhs);
