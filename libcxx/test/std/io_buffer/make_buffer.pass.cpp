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

// make_buffer: the buffer-creation functions (N4771 [buffer.creation]) for pointers, buffers, arrays, vectors,
// strings, string_views and spans; the size is in bytes and can be clamped.

#include <io_buffer>

#include <array>
#include <cassert>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace io = std::io;

static_assert(std::is_same_v<decltype(io::make_buffer(std::declval<void*>(), 1)), io::mutable_buffer>);
static_assert(std::is_same_v<decltype(io::make_buffer(std::declval<const void*>(), 1)), io::const_buffer>);
static_assert(std::is_same_v<decltype(io::make_buffer(std::declval<std::vector<int>&>())), io::mutable_buffer>);
static_assert(std::is_same_v<decltype(io::make_buffer(std::declval<const std::vector<int>&>())), io::const_buffer>);
static_assert(std::is_same_v<decltype(io::make_buffer(std::declval<std::string&>())), io::mutable_buffer>);
static_assert(std::is_same_v<decltype(io::make_buffer(std::declval<const std::string&>())), io::const_buffer>);
static_assert(std::is_same_v<decltype(io::make_buffer(std::declval<std::string_view>())), io::const_buffer>);
static_assert(std::is_same_v<decltype(io::make_buffer(std::declval<std::span<char>>())), io::mutable_buffer>);
static_assert(std::is_same_v<decltype(io::make_buffer(std::declval<std::span<const char>>())), io::const_buffer>);
static_assert(std::is_same_v<decltype(io::make_buffer(std::declval<int (&)[4]>())), io::mutable_buffer>);
static_assert(std::is_same_v<decltype(io::make_buffer(std::declval<const int (&)[4]>())), io::const_buffer>);
static_assert(std::is_same_v<decltype(io::make_buffer(std::declval<io::mutable_buffer>())), io::mutable_buffer>);
static_assert(std::is_same_v<decltype(io::make_buffer(std::declval<io::const_buffer>())), io::const_buffer>);

// A temporary vector would dangle: no mutable buffer from it.
template <class T>
concept makes_mutable = requires { requires std::is_same_v<decltype(io::make_buffer(std::declval<T>())), io::mutable_buffer>; };
static_assert(!makes_mutable<std::vector<int>>);
static_assert(makes_mutable<std::vector<int>&>);
static_assert(makes_mutable<std::span<int>>);

// Not for types that are not trivially copyable.
template <class T>
concept can_make = requires(T& t) { io::make_buffer(t); };
static_assert(can_make<std::vector<int>>);
static_assert(!can_make<std::vector<std::string>>);
static_assert(!can_make<int>);
static_assert(!can_make<std::vector<bool>>);

int main(int, char**) {
  // Pointer and size.
  {
    int x[4] = {};
    io::mutable_buffer m = io::make_buffer(x, sizeof(x));
    assert(m.data() == x && m.size() == 16);
    io::mutable_buffer mc = io::make_buffer(x, sizeof(x), 5);
    assert(mc.data() == x && mc.size() == 5);
    assert(io::make_buffer(x, sizeof(x), 1000).size() == 16);
    const int* cx = x;
    io::const_buffer c = io::make_buffer(cx, 8);
    assert(c.data() == x && c.size() == 8);
    assert(io::make_buffer(cx, 8, 3).size() == 3);
  }

  // Buffers pass through, optionally clamped.
  {
    char a[8];
    io::mutable_buffer m(a, 8);
    assert(io::make_buffer(m).size() == 8);
    assert(io::make_buffer(m, 3).size() == 3 && io::make_buffer(m, 3).data() == a);
    assert(io::make_buffer(m, 30).size() == 8);
    io::const_buffer c(a, 8);
    assert(io::make_buffer(c).size() == 8);
    assert(io::make_buffer(c, 3).size() == 3);
    assert(io::make_buffer(c, 0).size() == 0);
  }

  // Arrays: the size is in bytes, element types are arbitrary trivially copyable types.
  {
    int ia[5] = {};
    assert(io::make_buffer(ia).size() == 5 * sizeof(int) && io::make_buffer(ia).data() == ia);
    assert(io::make_buffer(ia, 7).size() == 7);
    const std::uint16_t ca[3] = {};
    io::const_buffer c = io::make_buffer(ca);
    assert(c.data() == ca && c.size() == 6);
    std::array<std::uint32_t, 4> sa{};
    assert(io::make_buffer(sa).size() == 16 && io::make_buffer(sa).data() == sa.data());
    const auto& csa = sa;
    assert(io::make_buffer(csa).size() == 16);
    assert(io::make_buffer(csa, 6).size() == 6);
    // A string literal includes its terminator, as in N4771.
    assert(io::make_buffer("abc").size() == 4);
  }

  // Vectors and strings.
  {
    std::vector<int> v(10);
    assert(io::make_buffer(v).size() == 40 && io::make_buffer(v).data() == v.data());
    assert(io::make_buffer(v, 6).size() == 6);
    const auto& cv = v;
    assert(io::make_buffer(cv).size() == 40);

    std::vector<int> empty;
    assert(io::make_buffer(empty).size() == 0 && io::make_buffer(empty).data() == nullptr);

    std::string s = "hello";
    assert(io::make_buffer(s).size() == 5 && io::make_buffer(s).data() == s.data());
    const std::string cs = "hello";
    assert(io::make_buffer(cs).size() == 5);
    assert(io::make_buffer(cs, 2).size() == 2);
    std::wstring ws = L"ab";
    assert(io::make_buffer(ws).size() == 2 * sizeof(wchar_t));

    std::string_view sv = "abcdef";
    assert(io::make_buffer(sv).size() == 6 && io::make_buffer(sv).data() == sv.data());
    assert(io::make_buffer(sv, 4).size() == 4);
    assert(io::make_buffer(std::string_view()).size() == 0);
  }

  // Spans, including a temporary span, which is a borrowed range.
  {
    std::vector<std::uint8_t> v(12);
    io::mutable_buffer m = io::make_buffer(std::span<std::uint8_t>(v).subspan(2, 6));
    assert(m.data() == v.data() + 2 && m.size() == 6);
    io::const_buffer c = io::make_buffer(std::span<const std::uint8_t>(v));
    assert(c.data() == v.data() && c.size() == 12);
  }

  // The result is a buffer sequence of the matching kind.
  {
    char a[4];
    static_assert(io::MutableBufferSequence<decltype(io::make_buffer(a))>);
    static_assert(!io::MutableBufferSequence<decltype(io::make_buffer("x"))>);
    assert(io::buffer_size(io::make_buffer(a)) == 4);
  }
  return 0;
}
