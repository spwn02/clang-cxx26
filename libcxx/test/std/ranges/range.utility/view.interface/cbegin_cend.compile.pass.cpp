//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

// <ranges>

// LWG 3766: view_interface::cbegin/cend are constrained on input_range (not just range).

#include <iterator>
#include <ranges>

struct Fwd : std::ranges::view_interface<Fwd> {
  int* begin();
  int* end();
};

// A range whose iterator is only an input_or_output_iterator, so it is a range but not an input_range.
struct OutputIterator {
  using difference_type = long;
  int& operator*() const;
  OutputIterator& operator++();
  void operator++(int);
  bool operator==(const OutputIterator&) const;
};
struct OutputOnly : std::ranges::view_interface<OutputOnly> {
  OutputIterator begin();
  OutputIterator end();
};
struct NotARange : std::ranges::view_interface<NotARange> {};

template <class V>
concept has_cbegin = requires(V& v) { v.cbegin(); };
template <class V>
concept has_cend = requires(V& v) { v.cend(); };

static_assert(has_cbegin<Fwd> && has_cend<Fwd>);
static_assert(!has_cbegin<const Fwd> && !has_cend<const Fwd>); // Fwd::begin() is not const
static_assert(!has_cbegin<NotARange> && !has_cend<NotARange>);
static_assert(std::ranges::range<OutputOnly> && !std::ranges::input_range<OutputOnly>);
static_assert(!has_cbegin<OutputOnly> && !has_cend<OutputOnly>);
