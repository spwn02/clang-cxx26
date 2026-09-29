// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s
// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -DTEST_X87_LONG_DOUBLE -std=c++26 -fsyntax-only -verify %s
// RUN: %clang_cc1 -triple powerpc64le-unknown-linux-gnu -DTEST_PPC_DOUBLE_DOUBLE -std=c++26 -fsyntax-only -verify %s
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

static_assert(__builtin_sqrt(4.0) == 2.0);
static_assert(__builtin_sqrtf(0x1p-148f) == 0x1p-74f);
#if !defined(TEST_PPC_DOUBLE_DOUBLE)
static_assert(__builtin_sqrtf16(0x1p-24f16) == 0x1p-12f16);
static_assert(__builtin_sqrtf128(4.0F128) == 2.0F128);
#endif
static_assert(__builtin_sqrtl(4.0L) == 2.0L);
#if defined(TEST_X87_LONG_DOUBLE)
static_assert(__builtin_sinl(1.0L) == 0.84147098480789650665L);
#endif
#if defined(TEST_PPC_DOUBLE_DOUBLE)
static_assert(__builtin_sinl(0x1.000000000000001p0L) !=
              __builtin_sinl(1.0L));
#endif
#if !defined(TEST_PPC_DOUBLE_DOUBLE)
static_assert(__builtin_sinf16((_Float16)0.0) == (_Float16)0.0);
static_assert(__builtin_sinf128((__float128)0.0) == (__float128)0.0);
static_assert(__builtin_erff128((__float128)0.0) == (__float128)0.0);
static_assert(__builtin_erfcf128((__float128)0.0) == (__float128)1.0);
static_assert(__builtin_tgammaf128((__float128)1.0) == (__float128)1.0);
static_assert(__builtin_lgammaf128((__float128)1.0) == (__float128)0.0);
static_assert(__builtin_powf16((_Float16)2.0, (_Float16)3.0) == (_Float16)8.0);
static_assert(__builtin_powf128((__float128)2.0, (__float128)3.0) == (__float128)8.0);
static_assert(__builtin_powf16((_Float16)2.0, (_Float16)-24.0) == (_Float16)0x1p-24);
#endif
static_assert(__builtin_pow(2.0, -1074.0) == 0x0.0000000000001p-1022);
static_assert(__builtin_pow(2.0, 10.0) == 1024.0);
static_assert(__builtin_exp(0.0) == 1.0);
static_assert(__builtin_log(1.0) == 0.0);
static_assert(__builtin_sin(-0.0) == 0.0 && __builtin_signbit(__builtin_sin(-0.0)));
static_assert(__builtin_cos(0.0) == 1.0);
static_assert(__builtin_atan2(-0.0, 1.0) == 0.0 &&
              __builtin_signbit(__builtin_atan2(-0.0, 1.0)));
static_assert(__builtin_atan2(0.0, 0.0) == 0.0);
constexpr double exp_overflow = __builtin_exp(10000.0); // expected-error {{must be initialized by a constant expression}}
constexpr double exp_underflow = __builtin_exp(-10000.0); // expected-error {{must be initialized by a constant expression}}
static_assert(__builtin_isinf(__builtin_sqrt(__builtin_huge_val())));
static_assert(__builtin_isnan(__builtin_sin(__builtin_nan(""))));
static_assert(__builtin_isnan(__builtin_pow(2.0, __builtin_nan(""))));
static_assert(__builtin_isnan(__builtin_atan2(2.0, __builtin_nan(""))));
static_assert(__builtin_isinf(__builtin_hypot(__builtin_nan(""),
                                              __builtin_huge_val())));
static_assert(__builtin_sin(0x1p60) >= -1.0 && __builtin_sin(0x1p60) <= 1.0);
static_assert(__builtin_sin(1.0) == 0x1.aed548f090ceep-1);
static_assert(__builtin_sin(0x1p60) == -0x1.a94adab06665cp-1);
static_assert(__builtin_exp2(3.0) == 8.0);
static_assert(__builtin_expm1(0.0) == 0.0);
static_assert(__builtin_log1p(0.0) == 0.0);
static_assert(__builtin_log2(8.0) == 3.0);
static_assert(__builtin_cbrt(-8.0) == -2.0);
static_assert(__builtin_hypot(3.0, 4.0) == 5.0);
static_assert(__builtin_tan(0.0) == 0.0);
static_assert(__builtin_asin(0.0) == 0.0);
static_assert(__builtin_acos(1.0) == 0.0);
static_assert(__builtin_atan(0.0) == 0.0);
static_assert(__builtin_sinh(-0.0) == 0.0 && __builtin_signbit(__builtin_sinh(-0.0)));
static_assert(__builtin_cosh(0.0) == 1.0);
static_assert(__builtin_tanh(-0.0) == 0.0 && __builtin_signbit(__builtin_tanh(-0.0)));
static_assert(__builtin_asinh(-0.0) == 0.0 && __builtin_signbit(__builtin_asinh(-0.0)));
static_assert(__builtin_acosh(1.0) == 0.0);
static_assert(__builtin_atanh(0.0) == 0.0);
static_assert(__builtin_erf(0.0) == 0.0);
static_assert(__builtin_erfc(0.0) == 1.0);
static_assert(__builtin_tgamma(1.0) == 1.0);
static_assert(__builtin_lgamma(1.0) == 0.0);

constexpr double negative_sqrt = __builtin_sqrt(-1.0); // expected-error {{must be initialized by a constant expression}}
constexpr double zero_log = __builtin_log(0.0); // expected-error {{must be initialized by a constant expression}}
constexpr double invalid_asin = __builtin_asin(2.0); // expected-error {{must be initialized by a constant expression}}
constexpr double zero_pow = __builtin_pow(0.0, -1.0); // expected-error {{must be initialized by a constant expression}}
constexpr double negative_log = __builtin_log(-1.0); // expected-error {{must be initialized by a constant expression}}
constexpr double bad_log1p = __builtin_log1p(-1.0); // expected-error {{must be initialized by a constant expression}}
constexpr double bad_acosh = __builtin_acosh(0.0); // expected-error {{must be initialized by a constant expression}}
constexpr double bad_atanh = __builtin_atanh(1.0); // expected-error {{must be initialized by a constant expression}}
constexpr double bad_acos = __builtin_acos(2.0); // expected-error {{must be initialized by a constant expression}}
