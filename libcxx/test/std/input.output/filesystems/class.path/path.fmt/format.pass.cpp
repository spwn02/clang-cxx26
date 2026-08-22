//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: availability-filesystem-missing

// TODO FMT This test should not require std::to_chars(floating-point)
// XFAIL: availability-fp_to_chars-missing

// <filesystem>

// template<class charT>
// struct formatter<filesystem::path, charT>;
//
// path-format-spec:
//   fill-and-align_opt width_opt ?_opt g_opt

#include <cassert>
#include <filesystem>
#include <format>
#include <string>

#include "make_string.h"
#include "test_macros.h"

#define SV(S) MAKE_STRING_VIEW(CharT, S)

namespace fs = std::filesystem;

template <class CharT>
void test_fmt() {
  fs::path p = "/usr/local/bin";
  std::basic_string<CharT> expected = p.string<CharT>();

  // *** basic formatting, no format-spec options ***
  assert(std::format(SV("{}"), p) == expected);

  // *** align-fill & width; default alignment is left, like other string types ***
  {
    std::basic_string<CharT> result = std::format(SV("{:20}"), p);
    assert(result.size() == 20);
    assert(result.compare(0, expected.size(), expected) == 0);
    assert(result.find(CharT(' '), expected.size()) == expected.size());
  }
  {
    std::basic_string<CharT> result = std::format(SV("{:*<20}"), p);
    assert(result.size() == 20);
    assert(result.compare(0, expected.size(), expected) == 0);
    for (std::size_t i = expected.size(); i < result.size(); ++i)
      assert(result[i] == CharT('*'));
  }
  {
    std::basic_string<CharT> result = std::format(SV("{:*>20}"), p);
    assert(result.size() == 20);
    std::size_t pad = result.size() - expected.size();
    for (std::size_t i = 0; i < pad; ++i)
      assert(result[i] == CharT('*'));
    assert(result.compare(pad, expected.size(), expected) == 0);
  }
  {
    std::basic_string<CharT> result = std::format(SV("{:*^21}"), p);
    assert(result.size() == 21);
    std::size_t before = (21 - expected.size()) / 2;
    assert(result.compare(before, expected.size(), expected) == 0);
  }
  // width smaller than the path: no padding is added.
  assert(std::format(SV("{:1}"), p) == expected);

  // *** the '?' option formats the path as an escaped (quoted) string ***
  {
    fs::path quoted = "a\"b";
    std::basic_string<CharT> result   = std::format(SV("{:?}"), quoted);
    std::basic_string<CharT> expected_quoted = std::format(SV("{:?}"), quoted.string<CharT>());
    assert(result == expected_quoted);
  }

  // *** the 'g' option formats the generic-format pathname ***
  assert(std::format(SV("{:g}"), p) == p.generic_string<CharT>());

  // *** '?' and 'g' can be combined, in that order ***
  {
    fs::path quoted = "a\"b";
    std::basic_string<CharT> result          = std::format(SV("{:?g}"), quoted);
    std::basic_string<CharT> expected_quoted = std::format(SV("{:?}"), quoted.generic_string<CharT>());
    assert(result == expected_quoted);
  }

  // *** empty path ***
  assert(std::format(SV("{}"), fs::path()) == SV(""));
}

int main(int, char**) {
  test_fmt<char>();
#ifndef TEST_HAS_NO_WIDE_CHARACTERS
  test_fmt<wchar_t>();
#endif

  return 0;
}
