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

// template<class charT, class traits = char_traits<charT>> class basic_spanbuf;

#include <cassert>
#include <ios>
#include <span>
#include <spanstream>
#include <utility>

#include "test_macros.h"

#ifndef __cpp_lib_spanstream
#  error "__cpp_lib_spanstream should be defined"
#endif
static_assert(__cpp_lib_spanstream == 202106L);

static_assert(std::is_same_v<std::spanbuf, std::basic_spanbuf<char>>);
static_assert(std::is_base_of_v<std::streambuf, std::spanbuf>);
static_assert(!std::is_copy_constructible_v<std::spanbuf>);
static_assert(!std::is_copy_assignable_v<std::spanbuf>);
static_assert(std::is_move_constructible_v<std::spanbuf>);
static_assert(std::is_move_assignable_v<std::spanbuf>);

// A class that exposes the protected pointers.
struct Probe : std::spanbuf {
  using std::spanbuf::spanbuf;
  char* eback_() const { return eback(); }
  char* gptr_() const { return gptr(); }
  char* egptr_() const { return egptr(); }
  char* pbase_() const { return pbase(); }
  char* pptr_() const { return pptr(); }
  char* epptr_() const { return epptr(); }
};

int main(int, char**) {
  char buf[8] = {'0', '1', '2', '3', '4', '5', '6', '7'};

  { // default: in | out, empty
    std::spanbuf sb;
    assert(sb.span().empty());
    assert(sb.span().data() == nullptr);
  }
  { // mode only
    std::spanbuf in_only(std::ios_base::in);
    assert(in_only.span().empty());
  }
  { // in | out: the get area is the whole buffer, the put area starts at the beginning
    Probe sb{std::span<char>(buf), std::ios_base::in | std::ios_base::out};
    assert(sb.eback_() == buf && sb.gptr_() == buf && sb.egptr_() == buf + 8);
    assert(sb.pbase_() == buf && sb.pptr_() == buf && sb.epptr_() == buf + 8);
    assert(sb.span().empty()); // out is set: the span is what has been written so far
    sb.sputc('x');
    sb.sputc('y');
    assert(sb.span().size() == 2 && sb.span().data() == buf);
    assert(buf[0] == 'x' && buf[1] == 'y');
    buf[0] = '0';
    buf[1] = '1';
  }
  { // ate: put at the end
    Probe sb{std::span<char>(buf, 4), std::ios_base::out | std::ios_base::ate};
    assert(sb.pptr_() == buf + 4 && sb.epptr_() == buf + 4);
    assert(sb.span().size() == 4);
    assert(sb.sputc('z') == std::spanbuf::traits_type::eof()); // no room
  }
  { // in only: span() is the whole buffer
    Probe sb{std::span<char>(buf, 5), std::ios_base::in};
    assert(sb.span().size() == 5);
    assert(sb.pbase_() == nullptr);
    assert(sb.in_avail() == 5);
    assert(sb.sbumpc() == '0');
    assert(sb.span().size() == 5);
  }
  { // span(s) resets both sequences
    Probe sb{std::span<char>(buf, 2), std::ios_base::in | std::ios_base::out};
    sb.sputc('a');
    sb.span(std::span<char>(buf + 2, 3));
    assert(sb.eback_() == buf + 2 && sb.gptr_() == buf + 2 && sb.egptr_() == buf + 5);
    assert(sb.pbase_() == buf + 2 && sb.pptr_() == buf + 2 && sb.epptr_() == buf + 5);
  }
  { // setbuf installs a new buffer
    std::spanbuf sb(std::ios_base::in);
    assert(sb.pubsetbuf(buf, 6) == &sb);
    assert(sb.span().size() == 6 && sb.span().data() == buf);
  }
  { // seekoff / seekpos on the get area
    std::spanbuf sb{std::span<char>(buf), std::ios_base::in};
    assert(sb.pubseekoff(3, std::ios_base::beg, std::ios_base::in) == 3);
    assert(sb.sgetc() == '3');
    assert(sb.pubseekoff(2, std::ios_base::cur, std::ios_base::in) == 5);
    assert(sb.pubseekoff(-1, std::ios_base::end, std::ios_base::in) == 7);
    assert(sb.pubseekoff(0, std::ios_base::end, std::ios_base::in) == 8);
    assert(sb.pubseekoff(1, std::ios_base::end, std::ios_base::in) == -1);   // past the end
    assert(sb.pubseekoff(-9, std::ios_base::end, std::ios_base::in) == -1);  // before the beginning
    assert(sb.pubseekoff(0, std::ios_base::cur, std::ios_base::in | std::ios_base::out) == -1); // both, cur: error
    assert(sb.pubseekoff(0, std::ios_base::beg, std::ios_base::out) == -1);  // not an output buffer
    assert(sb.pubseekoff(0, std::ios_base::beg, std::ios_base::openmode{}) == -1);
    assert(sb.pubseekpos(4, std::ios_base::in) == 4);
    assert(sb.sgetc() == '4');
    assert(sb.pubseekpos(9, std::ios_base::in) == -1);
  }
  { // seekoff on the put area; end is relative to what was written when only out is set
    std::spanbuf sb{std::span<char>(buf), std::ios_base::out};
    sb.sputc('a');
    sb.sputc('b');
    assert(sb.pubseekoff(0, std::ios_base::cur, std::ios_base::out) == 2);
    assert(sb.pubseekoff(0, std::ios_base::end, std::ios_base::out) == 2);
    assert(sb.pubseekoff(-1, std::ios_base::end, std::ios_base::out) == 1);
    assert(sb.span().size() == 1);
    assert(sb.pubseekpos(0, std::ios_base::out) == 0);
    assert(sb.span().empty());
  }
  { // move construction keeps the sequences
    Probe a{std::span<char>(buf), std::ios_base::in | std::ios_base::out};
    a.sputc('m');
    (void)a.sgetc();
    Probe b(std::move(a));
    assert(b.pbase_() == buf && b.pptr_() == buf + 1 && b.epptr_() == buf + 8);
    assert(b.eback_() == buf && b.egptr_() == buf + 8);
  }
  { // swap and move assignment
    char other[4] = {};
    std::spanbuf a{std::span<char>(buf), std::ios_base::in};
    std::spanbuf b{std::span<char>(other), std::ios_base::in};
    a.swap(b);
    assert(a.span().data() == other && b.span().data() == buf);
    swap(a, b);
    assert(a.span().data() == buf && b.span().data() == other);
    a = std::move(b);
    assert(a.span().data() == other);
  }
#if !defined(TEST_HAS_NO_WIDE_CHARACTERS)
  {
    wchar_t wbuf[4] = {};
    std::wspanbuf sb{std::span<wchar_t>(wbuf), std::ios_base::out};
    sb.sputc(L'w');
    assert(sb.span().size() == 1 && wbuf[0] == L'w');
  }
#endif
  return 0;
}
