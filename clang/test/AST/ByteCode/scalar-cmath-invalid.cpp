// RUN: %clang_cc1 -std=c++26 -verify=expected,classic %s
// RUN: %clang_cc1 -std=c++26 -fexperimental-new-constant-interpreter -verify=expected,ref %s

constexpr double invalid_fma = __builtin_fma(__builtin_inf(), 0.0, 1.0); // expected-error {{must be initialized by a constant expression}}
constexpr double signaling_fma = __builtin_fma(__builtin_nans(""), 1.0, 2.0); // expected-error {{must be initialized by a constant expression}}
constexpr double null_frexp = __builtin_frexp(1.0, nullptr); // expected-error {{must be initialized by a constant expression}} expected-note {{assignment to dereferenced null pointer is not allowed in a constant expression}}
constexpr int immutable = 0;
constexpr double const_frexp = __builtin_frexp(1.0, const_cast<int *>(&immutable)); // expected-error {{must be initialized by a constant expression}} expected-note {{a constant expression cannot modify an object that is visible outside that expression}}
constexpr double expired() {
  int *p;
  { int e = 0; p = &e; } // ref-note {{declared here}}
  return __builtin_frexp(1.0, p); // ref-note {{assignment to variable whose lifetime has ended}} classic-note {{assignment to object outside its lifetime is not allowed in a constant expression}}
}
constexpr double expired_frexp = expired(); // expected-error {{must be initialized by a constant expression}} expected-note {{in call to 'expired()'}}
constexpr double nonfinite_frexp = [] { int e; return __builtin_frexp(__builtin_inf(), &e); }(); // expected-error {{must be initialized by a constant expression}} expected-note {{subexpression not valid in a constant expression}} expected-note {{in call to}}
constexpr double nan_frexp = [] { int e; return __builtin_frexp(__builtin_nan(""), &e); }(); // expected-error {{must be initialized by a constant expression}} expected-note {{subexpression not valid in a constant expression}} expected-note {{in call to}}

constexpr double past_end_frexp = [] { int e[1]; return __builtin_frexp(1.0, e + 1); }(); // expected-error {{must be initialized by a constant expression}} expected-note {{assignment to dereferenced one-past-the-end pointer is not allowed in a constant expression}} expected-note {{in call to}}
