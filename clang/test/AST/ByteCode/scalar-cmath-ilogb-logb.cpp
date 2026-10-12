// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++26 -verify %s
// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++26 -fexperimental-new-constant-interpreter -verify %s

static_assert(__builtin_ilogbf(8.f) == 3);
static_assert(__builtin_ilogb(8.) == 3);
static_assert(__builtin_ilogbl(8.L) == 3);
static_assert(__builtin_logbf(8.f) == 3.f);
static_assert(__builtin_logb(8.) == 3.);
static_assert(__builtin_logbl(8.L) == 3.L);

static_assert(__builtin_ilogbf(-8.f) == 3);
static_assert(__builtin_ilogb(-8.) == 3);
static_assert(__builtin_ilogbl(-8.L) == 3);
static_assert(__builtin_logbf(-8.f) == 3.f);
static_assert(__builtin_logb(-8.) == 3.);
static_assert(__builtin_logbl(-8.L) == 3.L);

static_assert(__builtin_ilogbf(0x1p-149f) == -149);
static_assert(__builtin_ilogb(0x0.0000000000001p-1022) == -1074);
static_assert(__builtin_ilogbl(0x1p-16445L) == -16445);
static_assert(__builtin_logbf(0x1p-149f) == -149.f);
static_assert(__builtin_logb(0x0.0000000000001p-1022) == -1074.);
static_assert(__builtin_logbl(0x1p-16445L) == -16445.L);

// Classic evaluation deliberately leaves these exceptional inputs to run time.
static_assert(__builtin_ilogb(0.) == 0); // expected-error {{constant expression}}
static_assert(__builtin_ilogb(__builtin_inf()) == 0); // expected-error {{constant expression}}
static_assert(__builtin_ilogb(__builtin_nan("")) == 0); // expected-error {{constant expression}}
static_assert(__builtin_logb(0.) == 0.); // expected-error {{constant expression}}
static_assert(__builtin_logb(__builtin_inf()) == 0.); // expected-error {{constant expression}}
static_assert(__builtin_logb(__builtin_nan("")) == 0.); // expected-error {{constant expression}}

#if defined(__SIZEOF_FLOAT128__)
static_assert(__builtin_ilogbf128(8.Q) == 3);
static_assert(__builtin_logbf128(8.Q) == 3.Q);
#endif
