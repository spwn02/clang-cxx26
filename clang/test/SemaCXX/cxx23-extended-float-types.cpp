// RUN: %clang_cc1 -std=c++23 -triple x86_64-linux-gnu -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++26 -triple x86_64-linux-gnu -fsyntax-only -verify %s

// P1467R9: extended floating-point types. __float32/__float64 are the fork's
// spellings of std::float32_t/std::float64_t: distinct from float/double.

#if !defined(__CLANG_STDCPP_FLOAT16_T__) || !defined(__CLANG_STDCPP_FLOAT32_T__) || \
    !defined(__CLANG_STDCPP_FLOAT64_T__) || !defined(__CLANG_STDCPP_FLOAT128_T__) || \
    !defined(__CLANG_STDCPP_BFLOAT16_T__)
#error "missing predefined macro"
#endif
// The standard macros are left to the C++ library (libstdc++ misreads them).
#if defined(__STDCPP_FLOAT32_T__)
#error "unexpected"
#endif

static_assert(!__is_same(__float32, float));
static_assert(!__is_same(__float64, double));
static_assert(!__is_same(__float64, long double));
static_assert(sizeof(__float32) == 4 && sizeof(__float64) == 8);
static_assert(alignof(__float32) == 4 && alignof(__float64) == 8);
static_assert(__is_floating_point(__float32) && __is_floating_point(__float64));
static_assert(__is_arithmetic(__float32) && __is_signed(__float64));

// Literal suffixes.
static_assert(__is_same(decltype(1.0f32), __float32));
static_assert(__is_same(decltype(1.0f64), __float64));
static_assert(__is_same(decltype(1.0f16), _Float16));
static_assert(__is_same(decltype(1.0bf16), __bf16));
static_assert(__is_same(decltype(1.0f128), __float128));
static_assert(__is_same(decltype(1.0F32), __float32));
static_assert(1.5f32 == 1.5f && 0.1f64 == 0.1);
static_assert(0x1p-2f32 == 0.25f);

// Usual arithmetic conversions: equal rank, the extended type wins by subrank.
static_assert(__is_same(decltype(1.0f32 + 1.0f), __float32));
static_assert(__is_same(decltype(1.0f + 1.0f32), __float32));
static_assert(__is_same(decltype(1.0f64 + 1.0), __float64));
static_assert(__is_same(decltype(1.0 + 1.0f64), __float64));
static_assert(__is_same(decltype(1.0f32 + 1.0), double));
static_assert(__is_same(decltype(1.0f64 + 1.0f), __float64));
static_assert(__is_same(decltype(1.0f64 + 1.0L), long double));
static_assert(__is_same(decltype(1.0f16 + 1.0f32), __float32));
static_assert(__is_same(decltype(1.0bf16 + 1.0f64), __float64));
static_assert(__is_same(decltype(1.0f32 + 1), __float32));
static_assert(__is_same(decltype(1.0f16 + 1.0f16), _Float16));

// Unordered conversion ranks cannot be combined.
void unordered(_Float16 h, __bf16 b) {
  (void)(h + b); // expected-error {{unordered floating-point conversion ranks}}
  (void)(b * h); // expected-error {{unordered floating-point conversion ranks}}
}

// [over.ics.rank]p4.3
int f(__float32) { return 1; } // #f32
int f(__float64) { return 2; } // #f64
int f(long long) { return 3; } // #fll
int g(__float32) { return 1; }
int g(double) { return 2; }
void overloads() {
  float x = 0;
  double d = 0;
  __float32 f32 = 0;
  __float64 f64 = 0;
  static_assert(__is_same(decltype(f(x)), int));
  (void)f(x);   // float -> float32_t has equal rank: chosen
  (void)f(d);   // double -> float64_t
  (void)f(f32);
  (void)f(f64);
  _Float16 h = 0;
  (void)f(h);   // expected-error {{call to 'f' is ambiguous}}
  // expected-note@#f32 {{candidate function}}
  // expected-note@#f64 {{candidate function}}
  // expected-note@#fll {{candidate function}}
}

// Conversions between the types are implicit and ordinary.
void conv(float x, double d, __float32 a, __float64 b) {
  a = x; x = a; b = d; d = b; a = b; b = a;
  __float64 c = a + b;
  (void)c;
}

// Templates and specializations distinguish the types.
template <class T> struct is_f32 { static constexpr bool value = false; };
template <> struct is_f32<__float32> { static constexpr bool value = true; };
static_assert(is_f32<__float32>::value && !is_f32<float>::value);
