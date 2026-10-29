// RUN: %clang_cc1 -std=c++17 -triple x86_64-unknown-linux-gnu -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++26 -triple x86_64-unknown-linux-gnu -fsyntax-only -verify %s
// expected-no-diagnostics

// __builtin_fmaf128 is evaluated in a constant expression with a single rounding, like __builtin_fma.

static_assert(__builtin_fmaf128(2.0q, 3.0q, 1.0q) == 7.0q, "");

// (1 + e)^2 - (1 + 2e) = e^2 exactly; a multiplication followed by an addition rounds the product to 1 + 2e and gives 0.
constexpr __float128 e = __builtin_ldexpf128(1.0q, -112);
constexpr __float128 x = 1.0q + e;
constexpr __float128 fused = __builtin_fmaf128(x, x, -(1.0q + 2 * e));
static_assert(fused == __builtin_ldexpf128(1.0q, -224), "");
static_assert(x * x - (1.0q + 2 * e) == 0.0q, "");
