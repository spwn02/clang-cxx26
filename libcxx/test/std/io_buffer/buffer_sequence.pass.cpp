//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23, c++26
// ADDITIONAL_COMPILE_FLAGS: -fexperimental-library

// <io_buffer>

// ConstBufferSequence / MutableBufferSequence, buffer_type, buffer_sequence_begin/end, and the interoperation with
// std::ranges: a single buffer is a sequence of one but not a range; a range of buffers is a sequence.

#include <io_buffer>

#include <array>
#include <cassert>
#include <deque>
#include <forward_list>
#include <list>
#include <ranges>
#include <span>
#include <string>
#include <vector>

namespace io = std::io;

// A type that converts to const_buffer only.
struct only_const {
  operator io::const_buffer() const noexcept { return {}; }
};
// A type that converts to mutable_buffer only: not a MutableBuffer, since const_buffer is two conversions away.
struct only_mutable {
  operator io::mutable_buffer() const noexcept { return {}; }
};
// Converts to both.
struct both {
  operator io::const_buffer() const noexcept { return {}; }
  operator io::mutable_buffer() const noexcept { return {}; }
};

// ---- single buffers ----
static_assert(io::ConstBufferSequence<io::const_buffer>);
static_assert(io::ConstBufferSequence<io::mutable_buffer>);
static_assert(io::MutableBufferSequence<io::mutable_buffer>);
static_assert(!io::MutableBufferSequence<io::const_buffer>);
static_assert(io::ConstBufferSequence<const io::const_buffer&>);
static_assert(io::MutableBufferSequence<io::mutable_buffer&>);
static_assert(io::ConstBuffer<io::const_buffer> && io::ConstBuffer<io::mutable_buffer>);
static_assert(io::MutableBuffer<io::mutable_buffer> && !io::MutableBuffer<io::const_buffer>);

// P4100R1 8.4: a single buffer is NOT a std::ranges::range.
static_assert(!std::ranges::range<io::const_buffer>);
static_assert(!std::ranges::range<io::mutable_buffer>);
static_assert(!std::ranges::range<io::const_buffer&>);

// ---- ranges of buffers ----
static_assert(io::ConstBufferSequence<std::vector<io::const_buffer>>);
static_assert(io::ConstBufferSequence<std::vector<io::mutable_buffer>>);
static_assert(io::MutableBufferSequence<std::vector<io::mutable_buffer>>);
static_assert(!io::MutableBufferSequence<std::vector<io::const_buffer>>);
static_assert(io::ConstBufferSequence<const std::vector<io::const_buffer>&>);
static_assert(io::ConstBufferSequence<std::array<io::const_buffer, 3>>);
static_assert(io::MutableBufferSequence<std::array<io::mutable_buffer, 3>>);
static_assert(io::ConstBufferSequence<std::array<io::const_buffer, 0>>);
static_assert(io::ConstBufferSequence<std::list<io::const_buffer>>);
static_assert(io::ConstBufferSequence<std::deque<io::mutable_buffer>>);
static_assert(io::ConstBufferSequence<std::span<io::const_buffer>>);
static_assert(io::MutableBufferSequence<std::span<io::mutable_buffer>>);
static_assert(io::ConstBufferSequence<io::const_buffer_pair>);
static_assert(io::MutableBufferSequence<io::mutable_buffer_pair>);
static_assert(io::ConstBufferSequence<io::mutable_buffer_pair>); // every mutable sequence is a const sequence
static_assert(!io::MutableBufferSequence<io::const_buffer_pair>);
static_assert(io::ConstBufferSequence<std::vector<only_const>>);
static_assert(io::ConstBufferSequence<std::vector<both>> && io::MutableBufferSequence<std::vector<both>>);
static_assert(io::ConstBufferSequence<std::ranges::empty_view<io::const_buffer>>);
static_assert(io::ConstBufferSequence<decltype(std::views::all(std::declval<std::vector<io::const_buffer>&>()))>);

// ---- user-defined single buffers ----
static_assert(io::ConstBufferSequence<only_const> && !io::MutableBufferSequence<only_const>);
static_assert(io::ConstBuffer<both> && io::MutableBuffer<both>);
static_assert(io::MutableBufferSequence<both>);
static_assert(!io::MutableBuffer<only_mutable>);
static_assert(!io::MutableBufferSequence<only_mutable>);
static_assert(!io::ConstBufferSequence<only_mutable>);

// ---- negative cases ----
static_assert(!io::ConstBufferSequence<int>);
static_assert(!io::ConstBufferSequence<void>);
static_assert(!io::ConstBufferSequence<std::vector<int>>);
static_assert(!io::ConstBufferSequence<std::vector<std::byte>>);
static_assert(!io::ConstBufferSequence<std::span<std::byte>>);
static_assert(!io::ConstBufferSequence<std::string>);
static_assert(!io::ConstBufferSequence<char[4]>);
static_assert(!io::ConstBufferSequence<io::const_buffer*>);
static_assert(!io::ConstBufferSequence<std::forward_list<io::const_buffer>>); // forward only, not bidirectional
static_assert(!io::MutableBufferSequence<std::vector<int>>);
static_assert(!io::MutableBufferSequence<int>);
static_assert(!io::MutableBufferSequence<std::vector<only_const>>);

// ---- buffer_type ----
static_assert(std::is_same_v<io::buffer_type<io::mutable_buffer>, io::mutable_buffer>);
static_assert(std::is_same_v<io::buffer_type<io::const_buffer>, io::const_buffer>);
static_assert(std::is_same_v<io::buffer_type<io::mutable_buffer_pair>, io::mutable_buffer>);
static_assert(std::is_same_v<io::buffer_type<std::vector<io::const_buffer>>, io::const_buffer>);

// ---- the sequence of one ----
static_assert(std::is_same_v<decltype(io::buffer_sequence_begin(std::declval<const io::const_buffer&>())), const io::const_buffer*>);
static_assert(std::is_same_v<decltype(io::buffer_sequence_end(std::declval<io::mutable_buffer&>())), const io::mutable_buffer*>);
static_assert(noexcept(io::buffer_sequence_begin(std::declval<io::const_buffer&>())));
static_assert(noexcept(io::buffer_sequence_end(std::declval<std::vector<io::const_buffer>&>())));

int main(int, char**) {
  char a[8] = {};

  // A single buffer: begin() points at it, end() one past it.
  {
    io::const_buffer b(a, 8);
    auto first = io::buffer_sequence_begin(b);
    auto last  = io::buffer_sequence_end(b);
    assert(first == &b && last == &b + 1);
    int n = 0;
    for (auto it = first; it != last; ++it) {
      assert(it->size() == 8);
      ++n;
    }
    assert(n == 1);

    io::mutable_buffer m(a, 4);
    assert(io::buffer_sequence_end(m) - io::buffer_sequence_begin(m) == 1);
    assert(io::const_buffer(*io::buffer_sequence_begin(m)).size() == 4);
  }

  // A range: the CPOs forward to std::ranges::begin/end, const and non-const.
  {
    std::vector<io::const_buffer> v{{a, 1}, {a + 1, 2}, {a + 3, 3}};
    assert(io::buffer_sequence_begin(v) == v.begin());
    assert(io::buffer_sequence_end(v) == v.end());
    const auto& cv = v;
    assert(io::buffer_sequence_begin(cv) == cv.begin());
    assert(io::buffer_sequence_end(cv) == cv.end());

    std::array<io::mutable_buffer, 2> arr{};
    assert(io::buffer_sequence_end(arr) - io::buffer_sequence_begin(arr) == 2);
    std::list<io::const_buffer> l(3);
    assert(std::distance(io::buffer_sequence_begin(l), io::buffer_sequence_end(l)) == 3);

    // Standard ranges algorithms accept a vector of buffers like any other range.
    assert(std::ranges::distance(v) == 3);
    std::size_t bytes = 0;
    for (io::const_buffer b : v)
      bytes += b.size();
    assert(bytes == 6);
  }

  // Elements convert through the user-defined conversion.
  {
    std::vector<both> v(3);
    assert(io::buffer_length(v) == 3);
    assert(io::buffer_size(v) == 0);
  }
  return 0;
}
