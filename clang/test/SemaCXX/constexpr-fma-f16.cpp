// RUN: %clang_cc1 -std=c++17 -triple x86_64-unknown-linux-gnu -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++26 -triple x86_64-unknown-linux-gnu -fsyntax-only -verify %s
// expected-no-diagnostics

// __builtin_fmaf16 is evaluated in a constant expression with a single rounding, like __builtin_fma.

static_assert(__builtin_fmaf16(2.0, 3.0, 1.0) == 7.0, "");

// (1 + e)^2 - (1 + 2e) = e^2 for e = 2^-10: one rounding keeps the 2^-20, rounding the product first gives 0.
constexpr __fp16 e = 0.0009765625; // 2^-10
constexpr __fp16 x = 1.0 + e;
static_assert(__builtin_fmaf16(x, x, -(1.0 + 2 * e)) == 0.00000095367431640625, ""); // 2^-20
