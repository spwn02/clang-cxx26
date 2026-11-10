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

// const_buffer, mutable_buffer: construction, conversion, byte arithmetic (N4771 [buffer.mutable], [buffer.const],
// [buffer.arithmetic]).

#include <io_buffer>

#include <cassert>
#include <type_traits>

namespace io = std::io;

static_assert(std::is_nothrow_default_constructible_v<io::const_buffer>);
static_assert(std::is_nothrow_default_constructible_v<io::mutable_buffer>);
static_assert(std::is_trivially_copyable_v<io::const_buffer>);
static_assert(std::is_trivially_copyable_v<io::mutable_buffer>);
static_assert(std::is_nothrow_constructible_v<io::const_buffer, const void*, std::size_t>);
static_assert(std::is_nothrow_constructible_v<io::mutable_buffer, void*, std::size_t>);

// mutable -> const is implicit, const -> mutable does not exist.
static_assert(std::is_convertible_v<io::mutable_buffer, io::const_buffer>);
static_assert(!std::is_convertible_v<io::const_buffer, io::mutable_buffer>);
static_assert(!std::is_constructible_v<io::mutable_buffer, const void*, std::size_t>);

static_assert(std::is_same_v<decltype(io::const_buffer().data()), const void*>);
static_assert(std::is_same_v<decltype(io::mutable_buffer().data()), void*>);
static_assert(noexcept(io::const_buffer().data()) && noexcept(io::const_buffer().size()));
static_assert(noexcept(io::const_buffer() += 1) && noexcept(io::mutable_buffer() + 1));

// A buffer is a constant expression, including its byte arithmetic.
constexpr char g_bytes[10] = {};
constexpr io::const_buffer g_cb(g_bytes, 10);
static_assert(g_cb.data() == g_bytes && g_cb.size() == 10);
static_assert((g_cb + 4).size() == 6);
static_assert((g_cb + 4).data() == g_bytes + 4);
static_assert(io::const_buffer().data() == nullptr && io::const_buffer().size() == 0);

int main(int, char**) {
  char a[16] = {};

  // Default-constructed: null and empty.
  {
    io::mutable_buffer m;
    io::const_buffer c;
    assert(m.data() == nullptr && m.size() == 0);
    assert(c.data() == nullptr && c.size() == 0);
  }

  // Construction and observers.
  {
    io::mutable_buffer m(a, 16);
    assert(m.data() == a && m.size() == 16);
    io::const_buffer c(a, 8);
    assert(c.data() == a && c.size() == 8);
    io::const_buffer cm = m; // implicit
    assert(cm.data() == a && cm.size() == 16);
  }

  // operator+= advances by min(n, size()).
  {
    io::mutable_buffer m(a, 16);
    m += 4;
    assert(m.data() == a + 4 && m.size() == 12);
    m += 0;
    assert(m.data() == a + 4 && m.size() == 12);
    m += 1000; // clamped
    assert(m.data() == a + 16 && m.size() == 0);
    m += 1;
    assert(m.data() == a + 16 && m.size() == 0);

    io::const_buffer c(a, 16);
    io::const_buffer& r = (c += 10);
    assert(&r == &c);
    assert(c.data() == a + 10 && c.size() == 6);
    c += std::size_t(-1);
    assert(c.data() == a + 16 && c.size() == 0);
  }

  // operator+ in both operand orders, for both types; the operands are unchanged.
  {
    io::mutable_buffer m(a, 16);
    io::mutable_buffer m1 = m + 3;
    io::mutable_buffer m2 = 3 + m;
    assert(m1.data() == a + 3 && m1.size() == 13);
    assert(m2.data() == a + 3 && m2.size() == 13);
    assert(m.data() == a && m.size() == 16);
    assert((m + 99).size() == 0 && (m + 99).data() == a + 16);

    io::const_buffer c(a, 16);
    io::const_buffer c1 = c + 5;
    io::const_buffer c2 = 5 + c;
    assert(c1.data() == a + 5 && c1.size() == 11);
    assert(c2.data() == a + 5 && c2.size() == 11);
    assert(c.size() == 16);
    static_assert(std::is_same_v<decltype(m + 1), io::mutable_buffer>);
    static_assert(std::is_same_v<decltype(c + 1), io::const_buffer>);
  }

  return 0;
}
