// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23, c++26

#include <__meta/token_locations.h>
#include <cassert>
#include <meta>
#include <source_location>
#include <span>

int main(int, char**) {
  constexpr std::meta::token_sequence __tokens = ^^{ alpha + beta };
  constexpr auto __first = std::meta::__x::__token_begin(__tokens[0]);
  constexpr auto __first_end = std::meta::__x::__token_end(__tokens[0]);
  constexpr auto __last = std::meta::__x::__token_end(__tokens[2]);
  static_assert(__first.line() == 10);
  static_assert(__first.column() != 0);
  static_assert(__first_end.column() > __first.column());
  static_assert(std::meta::__x::__token_sequence_begin(__tokens).line() ==
                __first.line());
  static_assert(std::meta::__x::__token_sequence_end(__tokens).column() ==
                __last.column());

  constexpr std::span<const std::meta::token> __subrange(__tokens.data() + 1, 2);
  static_assert(std::meta::__x::__token_sequence_begin(__subrange).column() ==
                std::meta::__x::__token_begin(__tokens[1]).column());
  assert(__first.file_name()[0] != '\0');
}
