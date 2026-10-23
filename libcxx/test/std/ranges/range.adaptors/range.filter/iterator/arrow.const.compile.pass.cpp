//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

// LWG4112: has-arrow requires operator->() to be const-qualified, so the
// iterator of filter_view has no operator-> when the base iterator's is not.

#include <iterator>
#include <ranges>

struct NonConstArrowIt {
  using difference_type = int;
  using value_type      = int;
  int operator*() const;
  NonConstArrowIt& operator++();
  void operator++(int);
  int* operator->();
  bool operator==(const NonConstArrowIt&) const;
};

struct ConstArrowIt {
  using difference_type = int;
  using value_type      = int;
  int operator*() const;
  ConstArrowIt& operator++();
  void operator++(int);
  int* operator->() const;
  bool operator==(const ConstArrowIt&) const;
};

struct V1 : std::ranges::view_base {
  NonConstArrowIt begin() const;
  NonConstArrowIt end() const;
};

struct V2 : std::ranges::view_base {
  ConstArrowIt begin() const;
  ConstArrowIt end() const;
};

struct Pred {
  bool operator()(int) const;
};

template <class It>
concept has_arrow = requires(It it) { it.operator->(); };

static_assert(!has_arrow<std::ranges::iterator_t<std::ranges::filter_view<V1, Pred>>>);
static_assert(has_arrow<std::ranges::iterator_t<std::ranges::filter_view<V2, Pred>>>);
