// RUN: %clang_cc1 -std=c++26 -fconstexpr-steps=100000 -fsyntax-only %s

static_assert(__builtin_floor(3.75) == 3.0);
static_assert(__builtin_floor(-3.75) == -4.0);
static_assert(__builtin_ceil(3.25) == 4.0);
static_assert(__builtin_ceil(-3.25) == -3.0);
static_assert(__builtin_trunc(3.75) == 3.0);
static_assert(__builtin_trunc(-3.75) == -3.0);
static_assert(__builtin_round(2.5) == 3.0);
static_assert(__builtin_round(-2.5) == -3.0);
static_assert(__builtin_nearbyint(2.5) == 2.0);
static_assert(__builtin_nearbyint(-2.5) == -2.0);
static_assert(__builtin_rint(2.5) == 2.0);
static_assert(__builtin_rint(-2.5) == -2.0);
static_assert(__builtin_fmod(-5.5, 2.0) == -1.5);
static_assert(__builtin_remainder(5.5, 2.0) == -0.5);

static_assert(__builtin_lround(2.5) == 3);
static_assert(__builtin_lround(-2.5) == -3);
static_assert(__builtin_llround(-2.5) == -3);
static_assert(__builtin_llround(2.5) == 3);
static_assert(__builtin_lrint(2.5) == 2);
static_assert(__builtin_lrint(-2.5) == -2);
static_assert(__builtin_llrint(-2.5) == -2);
static_assert(__builtin_llrint(2.5) == 2);

static_assert(__builtin_copysign(1.0, __builtin_floor(-0.0)) == -1.0);
static_assert(__builtin_copysign(1.0, __builtin_ceil(-0.0)) == -1.0);
static_assert(__builtin_copysign(1.0, __builtin_trunc(-0.0)) == -1.0);
static_assert(__builtin_copysign(1.0, __builtin_round(-0.0)) == -1.0);
static_assert(__builtin_copysign(1.0, __builtin_nearbyint(-0.0)) == -1.0);
static_assert(__builtin_copysign(1.0, __builtin_rint(-0.0)) == -1.0);
static_assert(__builtin_copysign(1.0, __builtin_fmod(-0.0, 2.0)) == -1.0);
static_assert(__builtin_copysign(1.0, __builtin_remainder(-0.0, 2.0)) == -1.0);

static_assert(__builtin_floor(__builtin_huge_val()) == __builtin_huge_val());
static_assert(__builtin_ceil(__builtin_huge_val()) == __builtin_huge_val());
static_assert(__builtin_trunc(__builtin_huge_val()) == __builtin_huge_val());
static_assert(__builtin_round(__builtin_huge_val()) == __builtin_huge_val());
static_assert(__builtin_nearbyint(__builtin_huge_val()) == __builtin_huge_val());
static_assert(__builtin_rint(__builtin_huge_val()) == __builtin_huge_val());
static_assert(__builtin_isnan(__builtin_floor(__builtin_nan(""))));
static_assert(__builtin_isnan(__builtin_ceil(__builtin_nan(""))));
static_assert(__builtin_isnan(__builtin_trunc(__builtin_nan(""))));
static_assert(__builtin_isnan(__builtin_round(__builtin_nan(""))));
static_assert(__builtin_isnan(__builtin_nearbyint(__builtin_nan(""))));
static_assert(__builtin_isnan(__builtin_rint(__builtin_nan(""))));
static_assert(__builtin_isnan(__builtin_fmod(1.0, __builtin_nan(""))));
static_assert(__builtin_isnan(__builtin_fmod(__builtin_huge_val(), 1.0)));
static_assert(__builtin_isnan(
    __builtin_remainder(1.0, __builtin_nan(""))));
static_assert(
    __builtin_isnan(__builtin_remainder(__builtin_huge_val(), 1.0)));
