// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++20 -verify %s
// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++20 -fexperimental-new-constant-interpreter -verify %s
// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++23 -verify %s
// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++23 -fexperimental-new-constant-interpreter -verify %s
// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++26 -verify %s
// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++26 -fexperimental-new-constant-interpreter -verify %s
// expected-no-diagnostics

static_assert(__builtin_fma(2.0, 3.0, 1.0) == 7.0);
static_assert(__builtin_fma(0.1, 10.0, -1.0) != 0.0);
static_assert(__builtin_fmaf(2.0f, 3.0f, 1.0f) == 7.0f);
static_assert(__builtin_fmal(2.0L, 3.0L, 1.0L) == 7.0L);
static_assert(__builtin_isnan(__builtin_fma(__builtin_nan(""), 1.0, 2.0)));
static_assert(__builtin_fma(1.7976931348623157e308, 2.0, 0.0) == __builtin_inf());
static_assert(__builtin_ldexp(1.5, 4) == 24.0);
static_assert(__builtin_ldexpf(1.5f, -1) == 0.75f);
static_assert(__builtin_scalbn(1.0, 10) == 1024.0);
static_assert(__builtin_scalbln(1.0, 9223372036854775807L) == __builtin_inf());
static_assert(__builtin_scalbln(1.0, (-9223372036854775807L - 1)) == 0.0);
static_assert(__builtin_nextafter(1.0, 2.0) == 1.0000000000000002);
static_assert(__builtin_nextafter(1.0, 0.0) == 0.9999999999999999);
static_assert(__builtin_nextafter(0.0, 1.0) == 4.9406564584124654e-324);
static_assert(__builtin_signbit(__builtin_nextafter(0.0, -0.0)));
static_assert(__builtin_nexttowardf(1.0f, 1.0L + 1e-10L) == 1.0000001f);
static_assert(__builtin_nextup(1.0) == 1.0000000000000002);
static_assert(__builtin_nextdown(1.0) == 0.9999999999999999);
static_assert(__builtin_nextdown(__builtin_inf()) == 1.7976931348623157e308);
static_assert(__builtin_isnan(__builtin_nextup(__builtin_nan(""))));
constexpr bool frexp_ok() { int e; double m = __builtin_frexp(48.0, &e); return m == 0.75 && e == 6; }
static_assert(frexp_ok());
constexpr bool frexp_zero() { int e=5; double m = __builtin_frexp(-0.0, &e); return m==0.0 && __builtin_signbit(m) && e==0; }
static_assert(frexp_zero());
constexpr bool frexp_small() { int e; double m = __builtin_frexp(4.9406564584124654e-324, &e); return m==0.5 && e==-1073; }
static_assert(frexp_small());
static_assert(__builtin_isnan(__builtin_fmaximum(__builtin_nan(""), 1.0)));
static_assert(__builtin_isnan(__builtin_fminimum(1.0, __builtin_nan(""))));
static_assert(__builtin_fmaximum_num(__builtin_nan(""), 1.0)==1.0);
static_assert(__builtin_fminimum_num(1.0, __builtin_nan(""))==1.0);
static_assert(!__builtin_signbit(__builtin_fmaximum(-0.0, 0.0)));
static_assert(__builtin_signbit(__builtin_fminimum(0.0, -0.0)));

static_assert(__builtin_nextafter(-0.0, 0.0) == 0.0 && !__builtin_signbit(__builtin_nextafter(-0.0, 0.0)));
static_assert(__builtin_nextafter(4.9406564584124654e-324, 0.0) == 0.0);
static_assert(__builtin_nextafter(1.7976931348623157e308, __builtin_inf()) == __builtin_inf());
static_assert(__builtin_nextafter(__builtin_inf(), 0.0) == 1.7976931348623157e308);
static_assert(__builtin_isnan(__builtin_nextafter(__builtin_nan(""), 1.0)));
static_assert(__builtin_isnan(__builtin_nextafter(1.0, __builtin_nan(""))));
static_assert(__builtin_nextafterf(1.0f, 2.0f) == 1.0000001f);
static_assert(__builtin_nextafterl(1.0L, 2.0L) > 1.0L);
static_assert(__builtin_nexttoward(1.0, 2.0L) == 1.0000000000000002);
static_assert(__builtin_nexttowardl(1.0L, 2.0L) > 1.0L);
static_assert(__builtin_signbit(__builtin_nexttoward(0.0, -0.0L)));
static_assert(__builtin_nextup(-__builtin_inf()) == -1.7976931348623157e308);
static_assert(__builtin_nextdown(-__builtin_inf()) == -__builtin_inf());
static_assert(__builtin_nextup(__builtin_inf()) == __builtin_inf());
static_assert(__builtin_isnan(__builtin_nextdown(__builtin_nan(""))));
static_assert(__builtin_nextupf(0.0f) == 0x1p-149f);
static_assert(__builtin_nextdownl(1.0L) < 1.0L);
static_assert(__builtin_ldexpl(1.5L, 4) == 24.0L);
static_assert(__builtin_ldexp(1.0, -2000) == 0.0);
static_assert(__builtin_signbit(__builtin_ldexp(-0.0, 100)));
static_assert(__builtin_ldexp(__builtin_inf(), -100) == __builtin_inf());
static_assert(__builtin_scalbnf(1.0f, 10) == 1024.0f);
static_assert(__builtin_scalbnl(1.0L, 10) == 1024.0L);
static_assert(__builtin_scalblnf(1.0f, 10L) == 1024.0f);
static_assert(__builtin_scalblnl(1.0L, 10L) == 1024.0L);
constexpr bool frexp_variants() {
  int e = 0;
  float f = __builtin_frexpf(-0.1f, &e);
  if (f != -0.8f || e != -3) return false;
  long double l = __builtin_frexpl(48.0L, &e);
  return l == 0.75L && e == 6;
}
static_assert(frexp_variants());
static_assert(__builtin_isnan(__builtin_fmaximum(1.0, __builtin_nan(""))));
static_assert(__builtin_isnan(__builtin_fminimum(__builtin_nan(""), 1.0)));
static_assert(!__builtin_signbit(__builtin_fmaximum(0.0, -0.0)));
static_assert(__builtin_signbit(__builtin_fminimum(-0.0, 0.0)));
static_assert(__builtin_fmaximumf(1.0f, 2.0f) == 2.0f);
static_assert(__builtin_fminimuml(1.0L, 2.0L) == 1.0L);
static_assert(__builtin_fminimum_numf(__builtin_nanf(""), 1.0f) == 1.0f);
static_assert(__builtin_fmaximum_numl(1.0L, __builtin_nanl("")) == 1.0L);
static_assert(__builtin_issignaling(__builtin_nextup(__builtin_nans(""))));
static_assert(__builtin_issignaling(__builtin_nextdown(__builtin_nans(""))));
static_assert(__builtin_issignaling(__builtin_nextafter(__builtin_nans(""), 1.0)));

#if defined(__SIZEOF_FLOAT128__)
static_assert(__builtin_ldexpf128(1.5Q, 4) == 24.0Q);
static_assert(__builtin_scalbnf128(1.0Q, 10) == 1024.0Q);
static_assert(__builtin_scalblnf128(1.0Q, 10L) == 1024.0Q);
static_assert(__builtin_nextafterf128(1.0Q, 2.0Q) > 1.0Q);
static_assert(__builtin_nexttowardf128(1.0Q, 0.0Q) < 1.0Q);
constexpr bool frexp128() { int e = 0; return __builtin_frexpf128(48.0Q, &e) == 0.75Q && e == 6; }
static_assert(frexp128());
static_assert(__builtin_fmaximum_numf128(1.0Q, 2.0Q) == 2.0Q);
#endif
static_assert(__builtin_fminimum_numf16((_Float16)1, (_Float16)2) == (_Float16)1);

constexpr bool frexp_array() {
  int exponents[2];
  double m = __builtin_frexp(48.0, exponents + 1);
  return m == 0.75 && exponents[1] == 6;
}
static_assert(frexp_array());
