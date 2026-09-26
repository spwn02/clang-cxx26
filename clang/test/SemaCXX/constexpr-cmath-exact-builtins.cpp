// RUN: %clang_cc1 -std=c++23 -fsyntax-only -verify %s

// Constant folding of the P0533R9 <cmath> functions that need only exact arithmetic.

static_assert(__builtin_fdim(3.0, 1.0) == 2.0);
static_assert(__builtin_fdim(1.0, 3.0) == 0.0 && !__builtin_signbit(__builtin_fdim(-0.0, 3.0)));
static_assert(__builtin_isnan(__builtin_fdim(__builtin_nan(""), 1.0)));
static_assert(__builtin_fdimf(5.5f, 2.0f) == 3.5f);
static_assert(__builtin_fdiml(5.5L, 2.0L) == 3.5L);
static_assert(__builtin_fma(2.0, 3.0, 1.0) == 7.0);
static_assert(__builtin_fma(0.1, 10.0, -1.0) != 0.0);  // exact: single rounding
static_assert(__builtin_fmaf(2.0f, 3.0f, 1.0f) == 7.0f);
static_assert(__builtin_fmal(2.0L, 3.0L, 1.0L) == 7.0L);
static_assert(__builtin_ldexp(1.5, 4) == 24.0);
static_assert(__builtin_ldexpf(1.5f, -1) == 0.75f);
static_assert(__builtin_scalbn(1.0, 10) == 1024.0);
static_assert(__builtin_ldexp(1.0, 2000) == __builtin_inf());
static_assert(__builtin_ilogb(8.0) == 3 && __builtin_ilogbf(0.1f) == -4 && __builtin_ilogbl(1e300L) == 996);
static_assert(__builtin_nextafter(1.0, 2.0) == 1.0000000000000002);
static_assert(__builtin_nextafter(1.0, 0.0) == 0.9999999999999999);
static_assert(__builtin_nextafter(1.0, 1.0) == 1.0);
static_assert(__builtin_nextafterf(1.0f, 2.0f) == 1.0000001f);
static_assert(__builtin_nextafter(0.0, 1.0) == 4.9406564584124654e-324);
static_assert(__builtin_signbit(__builtin_nextafter(0.0, -0.0)) );
static_assert(__builtin_nexttoward(1.0, 2.0L) == 1.0000000000000002);
static_assert(__builtin_nexttowardf(1.0f, 1.0L + 1e-10L) == 1.0000001f);
static_assert(__builtin_nexttowardf(1.0f, 1.0L) == 1.0f);
constexpr bool frexp_ok() { int e = 0; double m = __builtin_frexp(48.0, &e); return m == 0.75 && e == 6; }
static_assert(frexp_ok());
constexpr bool frexp0() { int e = 5; double m = __builtin_frexp(0.0, &e); return m == 0.0 && e == 0; }
static_assert(frexp0());
constexpr bool frexpn() { int e = 0; double m = __builtin_frexp(-0.1, &e); return m == -0.8 && e == -3; }
static_assert(frexpn());
// FP_ILOGB0 is defined by the C library, so it is left to run time.
constexpr int bad = __builtin_ilogb(0.0); // expected-error {{must be initialized by a constant expression}}
constexpr double bad2 = [] { int e; return __builtin_frexp(__builtin_inf(), &e); }(); // expected-error {{must be initialized by a constant expression}}
// expected-note@-1 {{subexpression not valid in a constant expression}}
// expected-note@-2 {{in call to}}

constexpr bool modf_ok() {
  double i = 0;
  double f = __builtin_modf(-3.25, &i);
  if (f != -0.25 || i != -3.0)
    return false;
  f = __builtin_modf(-3.0, &i);
  if (f != 0.0 || !__builtin_signbit(f) || i != -3.0)
    return false;
  f = __builtin_modf(__builtin_inf(), &i);
  if (f != 0.0 || i != __builtin_inf())
    return false;
  float fi = 0;
  float ff = __builtin_modff(2.5f, &fi);
  long double li = 0;
  long double lf = __builtin_modfl(0.75L, &li);
  return ff == 0.5f && fi == 2.0f && lf == 0.75L && li == 0.0L;
}
static_assert(modf_ok());
