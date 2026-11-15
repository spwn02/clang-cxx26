// RUN: %clang -std=c++29 -nostdinc++ -isystem %S/../../../libcxx/include -idirafter %S/../../../build-libcxx/include/c++/v1 -fsyntax-only -Xclang -verify %s

#include <__meta/token_locations.h>

// expected-no-diagnostics

using std::meta::__x::__token_begin;
using std::meta::__x::__token_end;
using std::meta::__x::__token_sequence_begin;
using std::meta::__x::__token_sequence_end;

constexpr std::meta::token_sequence quoted = ^^{ alpha + beta };

// The token's source range includes its spelling; end is the exclusive column.
static_assert(__token_begin(quoted[0]).line() == 12);
static_assert(__token_begin(quoted[0]).column() == 50);
static_assert(__token_end(quoted[0]).column() == 55);
static_assert(__token_begin(quoted[1]).column() == 56);
static_assert(__token_end(quoted[1]).column() == 57);
static_assert(__token_sequence_begin(quoted).column() == 50);
static_assert(__token_sequence_end(quoted).column() == 62);
constexpr std::span<const std::meta::token> subrange(quoted.data() + 1, 2);
static_assert(__token_sequence_begin(subrange).column() == 56);
static_assert(__token_sequence_end(subrange).column() == 62);

constexpr std::meta::token_sequence annotation = ^^{ \(42) };
static_assert(__token_end(annotation[0]).line() ==
              __token_begin(annotation[0]).line());
static_assert(__token_end(annotation[0]).column() ==
              __token_begin(annotation[0]).column());

constexpr std::meta::token_sequence synthesized =
    std::meta::token_sequence{};
static_assert(__token_begin(std::meta::token{}).line() == 0);
static_assert(__token_end(std::meta::token{}).column() == 0);
static_assert(__token_begin(std::meta::token{}).file_name()[0] == '\0');
static_assert(__token_sequence_begin(synthesized).file_name()[0] == '\0');

__macro raw_location(std::meta::token_sequence tokens) {
  return ^^{ \(__token_begin(tokens[0]).line() * 1000 +
               __token_begin(tokens[0]).column()) };
}
static_assert(raw_location!(gamma) == 43029);
static_assert(__token_end(std::meta::token{}).file_name()[0] == '\0');
