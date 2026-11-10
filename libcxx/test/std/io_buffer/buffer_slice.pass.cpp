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

// Byte-granularity slicing: keep_prefix, remove_prefix, keep_suffix, remove_suffix and the copying prefix, suffix,
// sans_prefix, sans_suffix, for buffers, buffer pairs and user types that provide tag_invoke(slice_tag, ...).

#include <io_buffer>

#include <array>
#include <cassert>
#include <vector>

namespace io = std::io;

// A sequence type that opts in to slicing.
namespace user {
struct two_parts {
  io::const_buffer_pair p;
  const io::const_buffer* begin() const noexcept { return p.data(); }
  const io::const_buffer* end() const noexcept { return p.data() + 2; }
};
void tag_invoke(io::slice_tag, two_parts& t, io::slice_how how, std::size_t n) noexcept {
  io::tag_invoke(io::slice_tag{}, t.p, how, n);
}
} // namespace user

template <class S>
concept sliceable = requires(S& s) { io::keep_prefix(s, 1); };

static_assert(sliceable<io::const_buffer>);
static_assert(sliceable<io::mutable_buffer>);
static_assert(sliceable<io::const_buffer_pair>);
static_assert(sliceable<io::mutable_buffer_pair>);
// A std::vector of buffers has no slice customization and there is no generic fallback.
static_assert(!sliceable<std::vector<io::const_buffer>>);
static_assert(!sliceable<int>);
static_assert(noexcept(io::keep_prefix(std::declval<io::const_buffer&>(), 1)));

int main(int, char**) {
  char a[16] = {};

  // ---- single buffers ----
  {
    io::const_buffer b(a, 10);
    io::keep_prefix(b, 4);
    assert(b.data() == a && b.size() == 4);
    io::keep_prefix(b, 100); // clamped
    assert(b.data() == a && b.size() == 4);

    b = io::const_buffer(a, 10);
    io::remove_prefix(b, 3);
    assert(b.data() == a + 3 && b.size() == 7);
    io::remove_prefix(b, 100);
    assert(b.data() == a + 10 && b.size() == 0);

    b = io::const_buffer(a, 10);
    io::keep_suffix(b, 4);
    assert(b.data() == a + 6 && b.size() == 4);
    io::keep_suffix(b, 100);
    assert(b.data() == a + 6 && b.size() == 4);

    b = io::const_buffer(a, 10);
    io::remove_suffix(b, 4);
    assert(b.data() == a && b.size() == 6);
    io::remove_suffix(b, 100);
    assert(b.data() == a && b.size() == 0);

    io::mutable_buffer m(a, 10);
    io::keep_suffix(m, 2);
    assert(m.data() == a + 8 && m.size() == 2);
    io::remove_suffix(m, 1);
    assert(m.data() == a + 8 && m.size() == 1);
  }

  // ---- non-mutating forms ----
  {
    const io::const_buffer b(a, 10);
    assert(io::prefix(b, 3).size() == 3 && io::prefix(b, 3).data() == a);
    assert(io::suffix(b, 3).size() == 3 && io::suffix(b, 3).data() == a + 7);
    assert(io::sans_prefix(b, 3).size() == 7 && io::sans_prefix(b, 3).data() == a + 3);
    assert(io::sans_suffix(b, 3).size() == 7 && io::sans_suffix(b, 3).data() == a);
    assert(b.size() == 10 && b.data() == a);
    static_assert(std::is_same_v<decltype(io::prefix(b, 1)), io::const_buffer>);
  }

  // ---- pairs: the slice may span, shrink or drop the individual buffers ----
  {
    io::const_buffer_pair p{io::const_buffer(a, 4), io::const_buffer(a + 8, 6)}; // 4 + 6 bytes
    assert(io::buffer_size(p) == 10);

    auto q = io::prefix(p, 2);
    assert(q[0].data() == a && q[0].size() == 2 && q[1].size() == 0 && io::buffer_size(q) == 2);
    q = io::prefix(p, 4);
    assert(q[0].size() == 4 && q[1].size() == 0);
    q = io::prefix(p, 7);
    assert(q[0].size() == 4 && q[1].data() == a + 8 && q[1].size() == 3);
    q = io::prefix(p, 100);
    assert(io::buffer_size(q) == 10);
    q = io::prefix(p, 0);
    assert(io::buffer_size(q) == 0);

    q = io::sans_prefix(p, 1);
    assert(q[0].data() == a + 1 && q[0].size() == 3 && q[1].size() == 6);
    q = io::sans_prefix(p, 4);
    assert(q[0].size() == 0 && q[1].size() == 6);
    q = io::sans_prefix(p, 6);
    assert(q[0].size() == 0 && q[1].data() == a + 10 && q[1].size() == 4);
    q = io::sans_prefix(p, 100);
    assert(io::buffer_size(q) == 0);

    q = io::suffix(p, 3);
    assert(q[0].size() == 0 && q[1].data() == a + 11 && q[1].size() == 3);
    q = io::suffix(p, 8);
    assert(q[0].data() == a + 2 && q[0].size() == 2 && q[1].size() == 6);
    q = io::suffix(p, 100);
    assert(io::buffer_size(q) == 10);

    q = io::sans_suffix(p, 8);
    assert(q[0].data() == a && q[0].size() == 2 && q[1].size() == 0);
    q = io::sans_suffix(p, 2);
    assert(q[0].size() == 4 && q[1].size() == 4);
    q = io::sans_suffix(p, 100);
    assert(io::buffer_size(q) == 0);

    // The sliced pair is still a buffer sequence that copies correctly.
    char src[10] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
    char d[8] = {};
    io::mutable_buffer_pair mp{io::mutable_buffer(d, 3), io::mutable_buffer(d + 5, 3)};
    assert(io::buffer_copy(io::sans_prefix(mp, 2), io::const_buffer(src, 10)) == 4);
    assert(d[2] == '0' && d[5] == '1' && d[6] == '2' && d[7] == '3');
  }

  // ---- user type ----
  {
    user::two_parts t{{io::const_buffer(a, 8), io::const_buffer(a, 3)}};
    static_assert(sliceable<user::two_parts>);
    io::remove_prefix(t, 9);
    assert(t.p[0].size() == 0 && t.p[1].size() == 2);
    assert(io::buffer_size(io::sans_suffix(t, 1)) == 1);
    io::keep_prefix(t, 1);
    assert(io::buffer_size(t) == 1);
  }
  return 0;
}
