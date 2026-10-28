//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

// <cmath>

// std::fma of float16_t and bfloat16_t computes x * y + z as if to infinite precision and rounds once
// ([cmath.syn], [c.math.fma]). An implementation that rounds the result to float first and then to the
// 16-bit format rounds twice and is wrong when the float result lands exactly between two values of the
// narrow format. The reference here is exact integer arithmetic, so it knows nothing about floating point.

// REQUIRES: has-int128

#include <bit>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <stdfloat>

#include "test_macros.h"

using i128 = __int128;
using u128 = unsigned __int128;

template <int Mant, int ExpBits>
struct Fmt {
  static constexpr int bias = (1 << (ExpBits - 1)) - 1;
  static constexpr int emin = 1 - bias;
  struct Unpacked {
    bool neg;
    long long mant; // value = mant * 2^exp
    int exp;
  };
  static Unpacked unpack(unsigned u) { // finite values only
    const int e = (u >> Mant) & ((1 << ExpBits) - 1);
    const long long m = u & ((1 << Mant) - 1);
    if (e == 0)
      return {(u & 0x8000) != 0, m, emin - Mant};
    return {(u & 0x8000) != 0, m | (1ll << Mant), e - bias - Mant};
  }
  // Round the exact value M * 2^e0 (M > 0) to the nearest representable value, ties to even.
  static unsigned round(bool neg, u128 M, int e0) {
    int len = 0;
    for (u128 t = M; t != 0; t >>= 1)
      ++len;
    const int E       = len - 1 + e0; // the value is in [2^E, 2^(E+1))
    int qe            = (E < emin ? emin : E) - Mant; // exponent of one unit in the last place
    const int shift   = qe - e0;
    u128 Q;
    if (shift <= 0) {
      Q = M << -shift;
    } else {
      Q              = M >> shift;
      const u128 rem = M & ((u128(1) << shift) - 1), half = u128(1) << (shift - 1);
      if (rem > half || (rem == half && (Q & 1)))
        ++Q;
    }
    if (Q == (u128(1) << (Mant + 1))) { // rounding carried into the next binade
      Q >>= 1;
      ++qe;
    }
    unsigned bits;
    if (Q < (u128(1) << Mant)) {
      bits = static_cast<unsigned>(Q); // subnormal (or zero)
    } else {
      const int biased = qe + Mant + bias;
      if (biased >= (1 << ExpBits) - 1)
        bits = ((1u << ExpBits) - 1) << Mant; // overflow to infinity
      else
        bits = (static_cast<unsigned>(biased) << Mant) | static_cast<unsigned>(Q - (u128(1) << Mant));
    }
    return bits | (neg ? 0x8000u : 0u);
  }
  // exact a * b + c; returns false when the sum is exactly zero (the sign rules of zero are not what is tested)
  static bool exact_fma(unsigned a, unsigned b, unsigned c, unsigned& out) {
    const auto A = unpack(a), B = unpack(b), C = unpack(c);
    const long long pm = A.mant * B.mant;
    const int pe       = A.exp + B.exp;
    if (pm == 0 && C.mant == 0)
      return false;
    // align to the smaller exponent of the terms that are not zero
    const int e0 = pm == 0 ? C.exp : (C.mant == 0 ? pe : (pe < C.exp ? pe : C.exp));
    i128 P = static_cast<i128>(pm) << (pe - e0), S = static_cast<i128>(C.mant) << (C.exp - e0);
    if (A.neg != B.neg)
      P = -P;
    if (C.neg)
      S = -S;
    S += P;
    if (S == 0)
      return false;
    const bool neg = S < 0;
    out            = round(neg, static_cast<u128>(neg ? -S : S), e0);
    return true;
  }
};

struct Rng {
  std::uint64_t s = 0x9e3779b97f4a7c15ull;
  std::uint64_t next() {
    s ^= s << 13;
    s ^= s >> 7;
    s ^= s << 17;
    return s;
  }
};

// float16_t: every finite pattern. bfloat16_t: the exponent range is limited so that the exact sum fits in 128 bits.
template <class T, int Mant, int ExpBits>
void run(bool limit_range) {
  using F = Fmt<Mant, ExpBits>;
  auto bits = [](T v) { return std::bit_cast<std::uint16_t>(v); };
  auto from = [](unsigned u) { return std::bit_cast<T>(static_cast<std::uint16_t>(u)); };
  Rng rng;
  auto pick = [&] {
    const unsigned inf = ((1u << ExpBits) - 1) << Mant;
    for (;;) {
      unsigned u = static_cast<unsigned>(rng.next()) & 0xffff;
      if ((u & 0x7fff) >= inf)
        continue; // infinity and NaN
      if (limit_range && (u & 0x7fff) != 0) {
        const int e = (u >> Mant) & ((1 << ExpBits) - 1);
        if (e < F::bias - 10 || e > F::bias + 9)
          continue;
      }
      return u;
    }
  };
  for (int i = 0; i < 4000000; ++i) {
    const unsigned a = pick(), b = pick(), c = pick();
    unsigned want;
    if (!F::exact_fma(a, b, c, want))
      continue;
    assert(bits(std::fma(from(a), from(b), from(c))) == want);
  }
}

int main(int, char**) {
#ifdef __STDCPP_FLOAT16_T__
  // a * b + c = 2914.9998944..., which rounds to 2914; going through float gives 2915.0 and then ties to 2916
  assert(std::bit_cast<std::uint16_t>(std::fma(std::bit_cast<std::float16_t>(std::uint16_t(0xbea0)),
                                               std::bit_cast<std::float16_t>(std::uint16_t(0xe6e0)),
                                               std::bit_cast<std::float16_t>(std::uint16_t(0x86ec)))) == 0x69b1);
  run<std::float16_t, 10, 5>(false);
#endif
#ifdef __STDCPP_BFLOAT16_T__
  run<std::bfloat16_t, 7, 8>(true);
#endif
  return 0;
}
