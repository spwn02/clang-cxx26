// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify=cxx26 %s
// RUN: %clang_cc1 -std=c++23 -fsyntax-only -verify=cxx23 %s
// RUN: %clang_cc1 -std=c++20 -fsyntax-only -verify=old,cxx20 %s

// cxx26-no-diagnostics

// The C-library spellings of the <cmath> functions that [cmath.syn] makes
// constexpr (the float and long double variants such as fmaf, nextafterl and
// fmal) are constant-evaluated like their __builtin_ forms from C++23 on
// (P0533R9); nextup/nextdown are new in C++26. Before that they are plain
// library calls, except for the functions that were already folded.

extern "C" {
float fmaf(float, float, float);       // old-note {{declared here}}
long double fmal(long double, long double, long double);
float nextafterf(float, float);
long double nextafterl(long double, long double);
float fabsf(float);
int abs(int);
long labs(long);
long long llabs(long long);
double ceil(double);
double modf(double, double *);
long lround(double);
long long llround(double);
long lroundf(float);
long long llroundl(long double);
double frexp(double, int *);
double nextup(double);                 // cxx23-note {{declared here}} cxx20-note {{declared here}}
float nextupf(float);
long double nextupl(long double);
double nextdown(double);
double fmaximum(double, double);
double fminimum(double, double);
double fmaximum_num(double, double);
double fminimum_num(double, double);
float nextdownf(float);
long double nextdownl(long double);
}

// Folded in every mode (the declaration carries the 'constexpr' builtin attribute).
constexpr bool frexp_ok() { int e = 0; return frexp(48.0, &e) == 0.75 && e == 6; }
static_assert(frexp_ok());

#if __cplusplus >= 202302L
static_assert(fmaf(2.0f, 3.0f, 1.0f) == 7.0f);
static_assert(fmal(2.0L, 3.0L, 1.0L) == 7.0L);
static_assert(fmaf(0.1f, 10.0f, -1.0f) != 0.0f); // a single rounding
static_assert(nextafterf(1.0f, 2.0f) == 1.0000001f);
static_assert(nextafterl(1.0L, 0.0L) < 1.0L);
static_assert(fabsf(-2.5f) == 2.5f);
static_assert(abs(-3) == 3 && labs(-3L) == 3L && llabs(-3LL) == 3LL);
static_assert(ceil(1.5) == 2.0);
constexpr bool modf_ok() { double i = 0; return modf(2.5, &i) == 0.5 && i == 2.0; }
static_assert(modf_ok());
static_assert(lround(-2.5) == -3 && llround(2.5) == 3 && lroundf(1.4f) == 1 && llroundl(0.5L) == 1);
#else
constexpr float old_fmaf = fmaf(2.0f, 3.0f, 1.0f); // old-error {{must be initialized by a constant expression}} \
                                                   // old-note {{non-constexpr function 'fmaf' cannot be used in a constant expression}}
#endif

#if __cplusplus >= 202400L
static_assert(nextup(1.0) == 1.0000000000000002);
static_assert(nextdown(1.0) == 0.9999999999999999);
static_assert(nextupf(1.0f) == 1.0000001f);
static_assert(nextdownf(1.0f) < 1.0f);
static_assert(nextupl(1.0L) > 1.0L);
static_assert(nextdownl(1.0L) < 1.0L);
static_assert(nextup(0.0) == 4.9406564584124654e-324);
static_assert(nextup(-0.0) == 4.9406564584124654e-324);
static_assert(nextdown(0.0) == -4.9406564584124654e-324);
static_assert(nextup(__builtin_inf()) == __builtin_inf());
static_assert(nextdown(__builtin_inf()) == 1.7976931348623157e308);
static_assert(nextup(-__builtin_inf()) == -1.7976931348623157e308);
static_assert(nextdown(-__builtin_inf()) == -__builtin_inf());
static_assert(__builtin_isnan(nextup(__builtin_nan(""))));
static_assert(__builtin_isnan(nextdownf(__builtin_nanf(""))));

static_assert(fmaximum(1.0, 2.0) == 2.0);
static_assert(fminimum(1.0, 2.0) == 1.0);
static_assert(fmaximum_num(1.0, 2.0) == 2.0);
static_assert(fminimum_num(1.0, 2.0) == 1.0);
static_assert(__builtin_isnan(fmaximum(__builtin_nan(""), 1.0)));
static_assert(__builtin_isnan(fminimum(1.0, __builtin_nan(""))));
static_assert(fmaximum_num(__builtin_nan(""), 1.0) == 1.0);
static_assert(fminimum_num(1.0, __builtin_nan("")) == 1.0);
static_assert(!__builtin_signbit(fmaximum(-0.0, 0.0)) && __builtin_signbit(fminimum(0.0, -0.0)));
static_assert(__builtin_fmaximumf(1.0f, 2.0f) == 2.0f && __builtin_fminimuml(1.0L, 2.0L) == 1.0L);
#else
constexpr double pre26_nextup = nextup(1.0); // cxx23-error {{must be initialized by a constant expression}} cxx20-error {{must be initialized by a constant expression}} \
                                             // cxx23-note {{non-constexpr function 'nextup' cannot be used in a constant expression}} cxx20-note {{non-constexpr function 'nextup' cannot be used in a constant expression}}
#endif
// The __builtin_ forms are available in every mode.
static_assert(__builtin_nextup(1.0) == 1.0000000000000002);
static_assert(__builtin_nextdownf(1.0f) < 1.0f);
