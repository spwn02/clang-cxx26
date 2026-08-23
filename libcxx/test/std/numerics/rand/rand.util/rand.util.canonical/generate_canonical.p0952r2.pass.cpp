//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <random>

// P0952R2: A new specification for std::generate_canonical
//
// template<class RealType, size_t digits, class URBG>
//     RealType generate_canonical(URBG& g);
//
// Let r = numeric_limits<RealType>::radix, R = g.max()-g.min()+1,
// d = min(digits, numeric_limits<RealType>::digits), k = the smallest
// integer such that R^k >= r^d, and x = floor(R^k / r^d). Attempts (each
// consuming k invocations of g to form S = sum g_i * R^i) are made until
// S < x*r^d; the result is floor(S/x)/r^d.

#include <cassert>
#include <cstddef>
#include <limits>
#include <random>

#include "test_macros.h"
#include "truncate_fp.h"

// A URNG with a small, non-power-of-two range, driven by a fixed script of
// outputs. Used to force (and observe) the rejection-sampling loop that
// P0952R2 adds: R == 3 isn't a power of two, so generate_canonical must
// reject and retry some outcomes to stay uniform (see [rand.util.canonical]
// Note 1: only an exact power-of-two range guarantees a single attempt).
struct ScriptedEngine {
  using result_type = unsigned;

  static constexpr result_type min() { return 0; }
  static constexpr result_type max() { return 2; }

  result_type operator()() {
    assert(__i < __size);
    return __script[__i++];
  }

  const result_type* __script;
  std::size_t __size;
  std::size_t __i = 0;
};

int main(int, char**) {
  // digits == 0: r^0 == 1, so k (the smallest integer with R^k >= 1) is 0.
  // No invocations of g are made, and the result is trivially 0.
  {
    std::minstd_rand0 r;
    float f = std::generate_canonical<float, 0>(r);
    assert(f == 0.0f);
    // Confirm no draw was consumed: the engine is still at its initial
    // state, so the next output is the well-known first minstd_rand0 value.
    assert(r() == 16807u);
  }
  {
    std::minstd_rand0 r;
    double d = std::generate_canonical<double, 0>(r);
    assert(d == 0.0);
    assert(r() == 16807u);
  }

  // Forced rejection: with R == 3 and digits == 2, r^d == 4, k == 2 (3^1 < 4
  // <= 3^2 == 9), x == floor(9/4) == 2, threshold == x*r^d == 8. The only
  // combination of two draws from {0,1,2} whose weighted sum S == g0 + 3*g1
  // reaches 8 is (2, 2); every other combination is accepted. Script the
  // engine to produce the rejected pair first, then an accepted pair, and
  // confirm all 4 draws are consumed and the accepted pair's result is
  // returned.
  {
    static constexpr unsigned script[] = {2, 2, 0, 1};
    ScriptedEngine e{script, 4};
    double result = std::generate_canonical<double, 2>(e);
    assert(e.__i == 4); // both the rejected and the accepted attempt ran
    // Accepted attempt: g0 == 0, g1 == 1, S == 0 + 3*1 == 3, x == 2,
    // r^d == 4 -> floor(3/2)/4 == 1/4.
    assert(result == 0.25);
  }

  // A power-of-two range never rejects (Note 1): every attempt is accepted
  // on the first try, so this is really just an ordinary correctness check
  // computed independently of the implementation above.
  {
    static constexpr unsigned script[] = {3, 1};
    struct PowerOfTwoEngine {
      using result_type = unsigned;
      static constexpr result_type min() { return 0; }
      static constexpr result_type max() { return 3; } // R == 4
      result_type operator()() { return script[__i++]; }
      const unsigned* script;
      std::size_t __i = 0;
    } e{script};
    // r^d == 4 == R, so k == 1, x == 1, threshold == 4: single draw, exact.
    double result = std::generate_canonical<double, 2>(e);
    assert(e.__i == 1);
    assert(result == 3.0 / 4.0);
  }

  // Boundary values (digits-1, digits, digits+1) against a real standard
  // engine, cross-checked against an independent reimplementation of
  // [rand.util.canonical]'s formula (see the session notes for the script
  // used to derive these exact expected values from minstd_rand0's default
  // seed: draws 16807, 282475249, ... with R == 2147483646).
  {
    typedef std::minstd_rand0 E;
    typedef float F;
    E r;
    F f = std::generate_canonical<F, std::numeric_limits<F>::digits - 1>(r);
    assert(f == truncate_fp(F(65) / F(1 << 23)));
  }
  {
    typedef std::minstd_rand0 E;
    typedef float F;
    E r;
    F f = std::generate_canonical<F, std::numeric_limits<F>::digits>(r);
    assert(f == truncate_fp(F(132) / F(1 << 24)));
  }
  {
    typedef std::minstd_rand0 E;
    typedef float F;
    E r;
    F f = std::generate_canonical<F, std::numeric_limits<F>::digits + 1>(r);
    assert(f == truncate_fp(F(132) / F(1 << 24)));
  }
  {
    typedef std::minstd_rand0 E;
    typedef double F;
    E r;
    F f = std::generate_canonical<F, std::numeric_limits<F>::digits - 1>(r);
    assert(f == truncate_fp(F(592972605552112.0) / F(4503599627370496.0)));
  }
  {
    typedef std::minstd_rand0 E;
    typedef double F;
    E r;
    F f = std::generate_canonical<F, std::numeric_limits<F>::digits>(r);
    assert(f == truncate_fp(F(1187105627162056.0) / F(9007199254740992.0)));
  }
  {
    typedef std::minstd_rand0 E;
    typedef double F;
    E r;
    F f = std::generate_canonical<F, std::numeric_limits<F>::digits + 1>(r);
    assert(f == truncate_fp(F(1187105627162056.0) / F(9007199254740992.0)));
  }

  // mt19937_64's range is exactly 2^64 (max() - min() + 1 wraps to 0 in
  // 64-bit arithmetic) -- this is the overflow trap generate_canonical must
  // avoid when computing R. Just check the result lands in the required
  // [0, 1) range across a spread of digits values and float/double/long
  // double. long double is the discriminating case on this fork's target
  // (x86-64 Linux, 64-bit extended-precision mantissa): R == 2^64 and
  // digits == 64 together are the exact combination that a loose
  // bits(R)+digits static bound would wrongly reject, even though R^1 ==
  // 2^64 fits trivially and k == 1 here.
  {
    std::mt19937_64 r;
    for (int i = 0; i < 1000; ++i) {
      double d = std::generate_canonical<double, std::numeric_limits<double>::digits>(r);
      assert(d >= 0.0 && d < 1.0);
      float f = std::generate_canonical<float, std::numeric_limits<float>::digits>(r);
      assert(f >= 0.0f && f < 1.0f);
      long double ld = std::generate_canonical<long double, std::numeric_limits<long double>::digits>(r);
      assert(ld >= 0.0L && ld < 1.0L);
    }
    // The path a real user actually hits: uniform_real_distribution and
    // exponential_distribution both call generate_canonical internally.
    std::uniform_real_distribution<long double> dist(0.0L, 1.0L);
    for (int i = 0; i < 1000; ++i) {
      long double ld = dist(r);
      assert(ld >= 0.0L && ld < 1.0L);
    }
  }

  // A generator whose range is small relative to the requested precision
  // needs many draws per attempt (large k); make sure that path also stays
  // within [0, 1) and terminates.
  {
    std::minstd_rand0 r;
    for (int i = 0; i < 1000; ++i) {
      double d = std::generate_canonical<double, std::numeric_limits<double>::digits>(r);
      assert(d >= 0.0 && d < 1.0);
    }
  }

  return 0;
}
