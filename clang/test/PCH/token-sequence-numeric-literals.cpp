// RUN: %clang_cc1 -std=c++2d -I %S/../Reflection/Inputs -I %S/Inputs -emit-pch -o %t.pch -x c++-header %S/Inputs/token-sequence-numeric-header.h
// RUN: %clang_cc1 -std=c++2d -I %S/../Reflection/Inputs -I %S/Inputs -include-pch %t.pch -fsyntax-only -verify %s

// The literal tokens of a token sequence read back from a PCH are injected
// like the ones written in the translation unit (#496): every literal has its
// own NUL-terminated spelling, so "12" is not read as the start of "1234".
// expected-no-diagnostics

namespace std::meta {
consteval auto queue_injection(token_sequence) -> void;
}

consteval { std::meta::queue_injection(^^{ constexpr int p[] = { \(two_numbers) }; }); }
static_assert(p[0] == 12 && p[1] == 34);

consteval { std::meta::queue_injection(^^{ constexpr double q[] = { \(three_numbers) }; }); }
static_assert(q[0] == 7 && q[1] == 8.5 && q[2] == 16);
