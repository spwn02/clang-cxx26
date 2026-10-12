//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20 || c++23
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

// define_static_string / reflect_constant_string reflect a long sized contiguous
// string within the default constexpr step budget (no -fconstexpr-steps).

#include <meta>
#include <string>
#include <string_view>

inline constexpr int count = 100000;

template <class CharT>
struct source {
  CharT data[count + 1];
};

template <class CharT>
constexpr source<CharT> make_source() {
  source<CharT> result{};
  for (int i = 0; i < count; ++i)
    result.data[i] = static_cast<CharT>('a' + i % 26);
  return result;
}

constexpr auto char_source  = make_source<char>();
constexpr auto wchar_source = make_source<char16_t>();

template <class CharT>
consteval bool check(const CharT* p) {
  return p[0] == 'a' && p[count - 1] == static_cast<CharT>('a' + (count - 1) % 26) && p[count] == CharT{};
}

static_assert(check(std::define_static_string(std::string_view(char_source.data, count))));
static_assert(check(std::define_static_string(std::u16string_view(wchar_source.data, count))));
// Building the std::string itself also consumes steps, so use a shorter one.
static_assert(std::define_static_string(std::string(20000, 'x'))[19999] == 'x');
static_assert(std::meta::extent(std::meta::type_of(std::meta::reflect_constant_string(
                  std::string_view(char_source.data, count)))) == count + 1);

int main(int, char**) { return 0; }
