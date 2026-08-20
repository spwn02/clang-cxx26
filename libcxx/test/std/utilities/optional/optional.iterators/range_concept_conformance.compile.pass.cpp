//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <optional>

// P3168R2: optional and optional<T&> model contiguous_range and view;
// optional<T&> additionally models borrowed_range (its iterators outlive
// the optional, since they point into the referenced object, not into
// storage owned by the optional).

#include <optional>
#include <ranges>
#include <type_traits>

// optional<T>: contiguous, sized, common range; a view but not borrowed.
static_assert(std::ranges::contiguous_range<std::optional<int>>);
static_assert(std::ranges::contiguous_range<const std::optional<int>>);
static_assert(std::ranges::sized_range<std::optional<int>>);
static_assert(std::ranges::common_range<std::optional<int>>);
static_assert(std::ranges::view<std::optional<int>>);
static_assert(!std::ranges::borrowed_range<std::optional<int>>);
static_assert(!std::ranges::borrowed_range<const std::optional<int>>);

static_assert(std::is_same_v<std::ranges::iterator_t<std::optional<int>>, int*>);
static_assert(std::is_same_v<std::ranges::iterator_t<const std::optional<int>>, const int*>);

// optional<T&>: contiguous, sized, common range; both a view and a
// borrowed_range, since its iterators point at the referenced object.
static_assert(std::ranges::contiguous_range<std::optional<int&>>);
static_assert(std::ranges::contiguous_range<const std::optional<int&>>);
static_assert(std::ranges::sized_range<std::optional<int&>>);
static_assert(std::ranges::common_range<std::optional<int&>>);
static_assert(std::ranges::view<std::optional<int&>>);
static_assert(std::ranges::borrowed_range<std::optional<int&>>);
static_assert(std::ranges::borrowed_range<const std::optional<int&>>);

static_assert(std::is_same_v<std::ranges::iterator_t<std::optional<int&>>, int*>);
