//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20
// UNSUPPORTED: no-localization

// <spanstream>

// basic_ispanstream, basic_ospanstream, basic_spanstream

#include <cassert>
#include <ios>
#include <span>
#include <spanstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "test_macros.h"

static_assert(std::is_base_of_v<std::istream, std::ispanstream>);
static_assert(std::is_base_of_v<std::ostream, std::ospanstream>);
static_assert(std::is_base_of_v<std::iostream, std::spanstream>);
static_assert(!std::is_copy_constructible_v<std::ispanstream>);
static_assert(std::is_move_constructible_v<std::ospanstream>);
// A read-only range is accepted only by ispanstream.
static_assert(std::is_constructible_v<std::ispanstream, std::string_view>);
static_assert(std::is_constructible_v<std::ispanstream, std::span<const char>>);
static_assert(!std::is_constructible_v<std::ispanstream, std::string>); // not a borrowed range
static_assert(!std::is_constructible_v<std::ospanstream, std::string_view>);
static_assert(std::is_same_v<decltype(std::declval<const std::ispanstream&>().span()), std::span<const char>>);
static_assert(std::is_same_v<decltype(std::declval<const std::ospanstream&>().span()), std::span<char>>);
static_assert(std::is_same_v<decltype(std::declval<const std::spanstream&>().span()), std::span<char>>);
static_assert(std::is_same_v<decltype(std::declval<const std::spanstream&>().rdbuf()), std::spanbuf*>);

int main(int, char**) {
  { // ispanstream
    char buf[] = "12 34 hello";
    std::ispanstream is{std::span<char>(buf, 11)};
    int a, b;
    std::string w;
    is >> a >> b >> w;
    assert(a == 12 && b == 34 && w == "hello");
    assert(is.span().size() == 11);
    assert(is.rdbuf()->span().data() == buf);
    is.clear();
    is.seekg(3);
    is >> b;
    assert(b == 34);
    is.span(std::span<char>(buf, 2));
    is.clear();
    a = 0;
    is >> a;
    assert(a == 12 && is.eof());
  }
  { // ispanstream from a read-only borrowed range
    const char text[] = "42 43";
    std::ispanstream is{std::span<const char>(text, 5)};
    int x, y;
    is >> x >> y;
    assert(x == 42 && y == 43);
    std::ispanstream is2{std::string_view("7")};
    int z;
    is2 >> z;
    assert(z == 7);
    is2.span(std::string_view("99"));
    is2.clear();
    is2 >> z;
    assert(z == 99);
  }
  { // ospanstream
    char buf[32] = {};
    std::ospanstream os{std::span<char>(buf)};
    os << "value=" << 42 << ';';
    assert(os.span().size() == 9);
    assert(std::string_view(os.span().data(), os.span().size()) == "value=42;");
    os.seekp(6);
    os << 7;
    assert(std::string_view(buf, 9) == "value=72;");
    assert(os.span().size() == 7); // the span covers what precedes the put pointer
  }
  { // ospanstream: writing past the end fails
    char small[3];
    std::ospanstream os{std::span<char>(small)};
    os << "abcdef";
    assert(!os.good());
    assert(os.span().size() == 3);
    assert(std::string_view(small, 3) == "abc");
  }
  { // ospanstream ate
    char buf[8] = "ab";
    std::ospanstream os{std::span<char>(buf, 4), std::ios_base::ate};
    assert(os.span().size() == 4);
  }
  { // spanstream reads and writes
    char buf[16] = {};
    std::spanstream ss{std::span<char>(buf)};
    ss << "10 20";
    assert(ss.span().size() == 5);
    int a = 0, b = 0;
    ss >> a >> b;
    assert(a == 10 && b == 20);
  }
  { // moving the stream keeps working
    char buf[16];
    std::ospanstream a{std::span<char>(buf)};
    a << "hi";
    std::ospanstream b(std::move(a));
    b << '!';
    assert(std::string_view(b.span().data(), b.span().size()) == "hi!");
    assert(b.rdbuf()->span().size() == 3);
    std::ospanstream c{std::span<char>(buf, 0)};
    c = std::move(b);
    assert(c.span().size() == 3);
    swap(b, c);
    assert(b.span().size() == 3);
  }
#if !defined(TEST_HAS_NO_WIDE_CHARACTERS)
  {
    wchar_t wbuf[8] = {};
    std::wospanstream w{std::span<wchar_t>(wbuf)};
    w << L"ab" << 1;
    assert(w.span().size() == 3);
    std::wispanstream wi{std::span<wchar_t>(wbuf, 3)};
    std::wstring ws;
    wi >> ws;
    assert(ws == L"ab1");
  }
#endif
  return 0;
}
