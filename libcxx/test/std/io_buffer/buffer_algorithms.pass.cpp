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

// buffer_size, buffer_empty, buffer_length and buffer_copy (customization point objects), including scatter/gather
// across sequences with empty buffers in the middle, truncation, at_most, and the tag_invoke(size_tag) hook.

#include <io_buffer>

#include <array>
#include <cassert>
#include <cstring>
#include <list>
#include <string>
#include <vector>

namespace io = std::io;

// ---- they are function objects, not functions ----
static_assert(std::is_object_v<decltype(io::buffer_size)>);
static_assert(std::is_object_v<decltype(io::buffer_empty)>);
static_assert(std::is_object_v<decltype(io::buffer_length)>);
static_assert(std::is_object_v<decltype(io::buffer_copy)>);
static_assert(std::is_same_v<decltype(io::buffer_size(std::declval<io::const_buffer>())), std::size_t>);
static_assert(std::is_same_v<decltype(io::buffer_empty(std::declval<io::const_buffer>())), bool>);
static_assert(std::is_same_v<decltype(io::buffer_length(std::declval<io::const_buffer>())), std::size_t>);
static_assert(noexcept(io::buffer_size(std::declval<io::const_buffer>())));
static_assert(noexcept(io::buffer_copy(std::declval<io::mutable_buffer>(), std::declval<io::const_buffer>())));

// Not callable with anything that is not a buffer sequence, or with a const source as destination.
template <class F, class... Args>
concept callable = requires(F f, Args... args) { f(args...); };
static_assert(!callable<decltype(io::buffer_size), int>);
static_assert(!callable<decltype(io::buffer_size), std::vector<int>>);
static_assert(!callable<decltype(io::buffer_empty), std::string>);
static_assert(!callable<decltype(io::buffer_copy), io::const_buffer, io::const_buffer>);
static_assert(!callable<decltype(io::buffer_copy), io::mutable_buffer, int>);
static_assert(callable<decltype(io::buffer_copy), io::mutable_buffer, io::mutable_buffer>);
static_assert(callable<decltype(io::buffer_copy), io::mutable_buffer, io::const_buffer, std::size_t>);

// buffer_size and buffer_empty are constexpr.
constexpr io::const_buffer g_arr[3] = {io::const_buffer(nullptr, 0), io::const_buffer(nullptr, 0), io::const_buffer(nullptr, 0)};
static_assert(io::buffer_empty(g_arr[0]));
static_assert(io::buffer_size(g_arr[0]) == 0);
static_assert(io::buffer_size(io::const_buffer(g_arr, 7)) == 7);
static_assert(io::buffer_length(io::const_buffer(g_arr, 7)) == 1);

// A sequence with an O(1) size, through the tag_invoke(size_tag) customization.
namespace user {
struct rope {
  mutable int iterations = 0;
  std::array<io::const_buffer, 2> parts;
  const io::const_buffer* begin() const noexcept {
    ++iterations;
    return parts.data();
  }
  const io::const_buffer* end() const noexcept { return parts.data() + 2; }
};
std::size_t tag_invoke(io::size_tag, const rope&) noexcept { return 12345; }
} // namespace user
static_assert(io::ConstBufferSequence<user::rope>);

int main(int, char**) {
  char a[32];
  for (int i = 0; i < 32; ++i)
    a[i] = static_cast<char>('a' + i % 26);

  // ---- buffer_size / buffer_empty / buffer_length ----
  {
    io::const_buffer one(a, 5);
    assert(io::buffer_size(one) == 5 && io::buffer_length(one) == 1 && !io::buffer_empty(one));
    io::const_buffer none;
    assert(io::buffer_size(none) == 0 && io::buffer_length(none) == 1 && io::buffer_empty(none));

    std::vector<io::const_buffer> v;
    assert(io::buffer_size(v) == 0 && io::buffer_length(v) == 0 && io::buffer_empty(v));
    v = {{a, 100}, {a, 100}, {a, 100}};
    assert(io::buffer_size(v) == 300); // std::ranges::size would say 3
    assert(io::buffer_length(v) == 3);
    assert(!io::buffer_empty(v));

    std::vector<io::const_buffer> zeros(4);
    assert(io::buffer_size(zeros) == 0 && io::buffer_length(zeros) == 4 && io::buffer_empty(zeros));
    zeros[3] = io::const_buffer(a, 1);
    assert(!io::buffer_empty(zeros) && io::buffer_size(zeros) == 1);

    std::list<io::mutable_buffer> l{{a, 2}, {a, 0}, {a, 3}};
    assert(io::buffer_size(l) == 5 && io::buffer_length(l) == 3);

    io::const_buffer_pair p{io::const_buffer(a, 3), io::const_buffer(a, 4)};
    assert(io::buffer_size(p) == 7 && io::buffer_length(p) == 2);
  }

  // ---- the size_tag customization is used, and the range is not iterated ----
  {
    user::rope r{0, {io::const_buffer(a, 1), io::const_buffer(a, 2)}};
    assert(io::buffer_size(r) == 12345);
    assert(r.iterations == 0);
    assert(io::buffer_length(r) == 2); // not customized: iterates
    assert(r.iterations == 1);
  }

  // ---- buffer_copy: single to single ----
  {
    char d[8];
    std::memset(d, '.', 8);
    assert(io::buffer_copy(io::mutable_buffer(d, 8), io::const_buffer(a, 3)) == 3);
    assert(std::memcmp(d, "abc.....", 8) == 0);

    // The destination is smaller: truncated.
    std::memset(d, '.', 8);
    assert(io::buffer_copy(io::mutable_buffer(d, 2), io::const_buffer(a, 8)) == 2);
    assert(std::memcmp(d, "ab......", 8) == 0);

    // at_most limits.
    std::memset(d, '.', 8);
    assert(io::buffer_copy(io::mutable_buffer(d, 8), io::const_buffer(a, 8), 4) == 4);
    assert(std::memcmp(d, "abcd....", 8) == 0);
    assert(io::buffer_copy(io::mutable_buffer(d, 8), io::const_buffer(a, 8), 0) == 0);
    assert(io::buffer_copy(io::mutable_buffer(d, 8), io::const_buffer(a, 8), 100) == 8);

    // Empty on either side, including null buffers.
    assert(io::buffer_copy(io::mutable_buffer(), io::const_buffer(a, 8)) == 0);
    assert(io::buffer_copy(io::mutable_buffer(d, 8), io::const_buffer()) == 0);
    assert(io::buffer_copy(io::mutable_buffer(), io::const_buffer()) == 0);

    // A mutable buffer is also a source.
    std::memset(d, '.', 8);
    char s[4] = {'w', 'x', 'y', 'z'};
    assert(io::buffer_copy(io::mutable_buffer(d, 8), io::mutable_buffer(s, 4)) == 4);
    assert(std::memcmp(d, "wxyz....", 8) == 0);
  }

  // ---- buffer_copy: gather (many sources, one destination), empty buffers in the middle ----
  {
    char d[16];
    std::memset(d, '.', 16);
    const char* s0 = "AAA";
    const char* s1 = "BB";
    const char* s2 = "CCCC";
    std::vector<io::const_buffer> src{{s0, 3}, {nullptr, 0}, {s1, 2}, {s1, 0}, {s2, 4}};
    assert(io::buffer_copy(io::mutable_buffer(d, 16), src) == 9);
    assert(std::memcmp(d, "AAABBCCCC.......", 16) == 0);

    // Truncation in the middle of a source buffer.
    std::memset(d, '.', 16);
    assert(io::buffer_copy(io::mutable_buffer(d, 4), src) == 4);
    assert(std::memcmp(d, "AAAB............", 16) == 0);

    // at_most that ends exactly on a buffer boundary and one that ends inside one.
    std::memset(d, '.', 16);
    assert(io::buffer_copy(io::mutable_buffer(d, 16), src, 5) == 5);
    assert(std::memcmp(d, "AAABB...........", 16) == 0);
    std::memset(d, '.', 16);
    assert(io::buffer_copy(io::mutable_buffer(d, 16), src, 7) == 7);
    assert(std::memcmp(d, "AAABBCC.........", 16) == 0);
  }

  // ---- buffer_copy: scatter (one source, many destinations), empty buffers in the middle ----
  {
    char d0[3], d1[1], d2[5];
    std::memset(d0, '.', 3);
    std::memset(d1, '.', 1);
    std::memset(d2, '.', 5);
    std::array<io::mutable_buffer, 5> dst{
        io::mutable_buffer(d0, 2), io::mutable_buffer(), io::mutable_buffer(d1, 0), io::mutable_buffer(d1, 1),
        io::mutable_buffer(d2, 5)};
    const char* src = "0123456789";
    assert(io::buffer_copy(dst, io::const_buffer(src, 10)) == 8);
    assert(std::memcmp(d0, "01.", 3) == 0);
    assert(d1[0] == '2');
    assert(std::memcmp(d2, "34567", 5) == 0);
  }

  // ---- buffer_copy: scatter and gather at once, with different boundaries ----
  {
    char d0[3], d1[4], d2[10];
    std::memset(d0, '.', 3);
    std::memset(d1, '.', 4);
    std::memset(d2, '.', 10);
    std::vector<io::mutable_buffer> dst{{d0, 3}, {d1, 4}, {d2, 10}};
    const char* x = "xxxxx";
    const char* y = "yyyyyy";
    std::list<io::const_buffer> src{{x, 5}, {y, 6}};
    assert(io::buffer_copy(dst, src) == 11);
    assert(std::memcmp(d0, "xxx", 3) == 0);
    assert(std::memcmp(d1, "xxyy", 4) == 0);
    assert(std::memcmp(d2, "yyyy......", 10) == 0);
  }

  // ---- buffer_copy: a buffer pair on either side (the wrap-around shape) ----
  {
    char d[8];
    std::memset(d, '.', 8);
    io::mutable_buffer_pair dst{io::mutable_buffer(d + 5, 3), io::mutable_buffer(d, 5)};
    io::const_buffer_pair src{io::const_buffer(a, 4), io::const_buffer(a + 10, 4)};
    assert(io::buffer_copy(dst, src) == 8);
    assert(std::memcmp(d + 5, "abc", 3) == 0);
    // src is "abcd" then "klmn"; dst fills d[5..8) first, then d[0..5).
    assert(std::memcmp(d + 5, "abc", 3) == 0);
    assert(std::memcmp(d, "dklmn", 5) == 0);
  }

  // ---- buffer_copy returns min(size(dest), size(src), at_most) ----
  {
    char d[10];
    for (std::size_t dn : {0, 1, 5, 10})
      for (std::size_t sn : {0, 1, 7, 10})
        for (std::size_t am : {std::size_t(0), std::size_t(3), std::size_t(8), std::size_t(-1)}) {
          std::size_t expected = dn < sn ? dn : sn;
          expected             = expected < am ? expected : am;
          assert(io::buffer_copy(io::mutable_buffer(d, dn), io::const_buffer(a, sn), am) == expected);
        }
  }
  return 0;
}
