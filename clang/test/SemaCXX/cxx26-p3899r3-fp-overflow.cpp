// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify -fexperimental-new-constant-interpreter -DBYTECODE %s
// RUN: %clang_cc1 -std=c++11 -fsyntax-only -verify %s

// P3899R3, [expr.const.core]: an arithmetic expression is not a core constant
// expression if all operands are finite and the result is not finite, if an
// operand is a signaling NaN, or if no operand is a NaN and the result is.

constexpr double inf = __builtin_inf();
constexpr double nan = __builtin_nan("");
constexpr float finf = __builtin_inff();

constexpr double overflow = 1e308 * 10; // expected-error {{must be initialized by a constant expression}} \
                                        // expected-note {{floating point arithmetic produces an infinity}}
constexpr float foverflow = 3.0e38f * 2; // expected-error {{must be initialized by a constant expression}} \
                                         // expected-note {{floating point arithmetic produces an infinity}}
constexpr double add_overflow = 1.7e308 + 1.7e308; // expected-error {{must be initialized by a constant expression}} \
                                                   // expected-note {{floating point arithmetic produces an infinity}}
constexpr double sub_overflow = -1.7e308 - 1.7e308; // expected-error {{must be initialized by a constant expression}} \
                                                    // expected-note {{floating point arithmetic produces an infinity}}
constexpr double div_overflow = 1e308 / 1e-308; // expected-error {{must be initialized by a constant expression}} \
                                                // expected-note {{floating point arithmetic produces an infinity}}

// An infinite operand is fine.
constexpr double inf2 = inf * 2;
constexpr double inf3 = -inf + 5;
constexpr double inf4 = inf + inf;
static_assert(inf2 == inf && inf3 == -inf && inf4 == inf, "");

// Underflow and inexact results are fine.
constexpr double tiny = 1e-308 / 1e308;
static_assert(tiny == 0.0, "");
constexpr double inexact = 1.0 / 3.0;

// A NaN produced from operands that are not NaNs.
constexpr double nan1 = inf * 0; // expected-error {{must be initialized by a constant expression}} \
                                 // expected-note {{floating point arithmetic produces a NaN}}
constexpr double nan2 = inf - inf; // expected-error {{must be initialized by a constant expression}} \
                                   // expected-note {{floating point arithmetic produces a NaN}}
constexpr double nan3 = inf / inf; // expected-error {{must be initialized by a constant expression}} \
                                   // expected-note {{floating point arithmetic produces a NaN}}
constexpr float fnan = finf - finf; // expected-error {{must be initialized by a constant expression}} \
                                    // expected-note {{floating point arithmetic produces a NaN}}

// A quiet NaN operand propagates.
constexpr double nan4 = nan * 2;
constexpr double nan5 = 1 + nan;
constexpr double nan6 = inf - nan;
constexpr double nan7 = -nan;
static_assert(nan4 != nan4 && nan5 != nan5 && nan6 != nan6 && nan7 != nan7, "");

// A signaling NaN operand is not a constant expression.
constexpr double snan = __builtin_nans("");
constexpr double s1 = snan + 1; // expected-error {{must be initialized by a constant expression}} \
                                // expected-note {{floating point arithmetic produces a NaN}}
constexpr double s2 = 2 * snan; // expected-error {{must be initialized by a constant expression}} \
                                // expected-note {{floating point arithmetic produces a NaN}}
constexpr double s3 = -snan; // expected-error {{must be initialized by a constant expression}} \
                             // expected-note {{floating point arithmetic produces a NaN}}
#ifndef BYTECODE // FIXME: the bytecode interpreter does not check the operand of a unary plus
constexpr double s4 = +snan; // expected-error {{must be initialized by a constant expression}} \
                             // expected-note {{floating point arithmetic produces a NaN}}
#endif

// Division by zero is undefined.
constexpr double z = 1.0 / 0.0; // expected-error {{must be initialized by a constant expression}} \
                                // expected-note {{division by zero}}

#if __cplusplus >= 201402L
// Compound assignment is defined in terms of arithmetic expressions.
constexpr double accumulate() {
  double d = 1e308;
  d *= 10; // expected-note {{floating point arithmetic produces an infinity}}
  return d;
}
constexpr double acc = accumulate(); // expected-error {{must be initialized by a constant expression}} \
                                     // expected-note {{in call to 'accumulate()'}}
constexpr double increments() {
  double d = inf;
  d += 1;
  d *= 2;
  return d;
}
static_assert(increments() == inf, "");
#endif
