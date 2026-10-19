// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s

// __builtin_type_order is a std::strong_ordering, <compare> is needed as for <=>.
constexpr auto order = __builtin_type_order(int, char); // expected-error {{cannot use builtin operator '<=>' because type 'std::strong_ordering' was not found; include <compare>}}
