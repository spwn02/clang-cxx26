// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___CHARCONV_FLOAT128_H
#define _LIBCPP___CHARCONV_FLOAT128_H

#include <__config>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 23 && defined(__SIZEOF_INT128__)

// Conversions between IEEE binary128 bit patterns and text (P1467R9: std::to_chars and std::from_chars for the
// extended floating-point types). Everything is exact: a small arbitrary precision integer replaces the table driven
// algorithms used for the narrower types, which keeps the implementation independent of the library binary (speed is
// not a goal for this type).
namespace __f128 {


using __u32 = unsigned int;
using __u64 = unsigned long long;
using __u128 = unsigned __int128;

// ---------------------------------------------------------------------------------------------------------------------
// Fixed capacity unsigned big integer (little endian 32-bit limbs).
// ---------------------------------------------------------------------------------------------------------------------
template <int _Cap>
struct __bignum {
  __u32 __l[_Cap];
  int __n = 0;

  _LIBCPP_HIDE_FROM_ABI inline void __zero() { __n = 0; }
  _LIBCPP_HIDE_FROM_ABI inline bool __is_zero() const { return __n == 0; }
  _LIBCPP_HIDE_FROM_ABI inline void __trim() {
    while (__n > 0 && __l[__n - 1] == 0)
      --__n;
  }
  _LIBCPP_HIDE_FROM_ABI inline void __set(__u128 __v) {
    __n = 0;
    while (__v) {
      __l[__n++] = static_cast<__u32>(__v);
      __v >>= 32;
    }
  }
  _LIBCPP_HIDE_FROM_ABI inline void __mul_small(__u32 __m) {
    __u64 __c = 0;
    for (int __i = 0; __i < __n; ++__i) {
      __c += static_cast<__u64>(__l[__i]) * __m;
      __l[__i] = static_cast<__u32>(__c);
      __c >>= 32;
    }
    if (__c)
      __l[__n++] = static_cast<__u32>(__c);
  }
  _LIBCPP_HIDE_FROM_ABI inline void __add_small(__u32 __a) {
    __u64 __c = __a;
    for (int __i = 0; __i < __n && __c; ++__i) {
      __c += __l[__i];
      __l[__i] = static_cast<__u32>(__c);
      __c >>= 32;
    }
    if (__c)
      __l[__n++] = static_cast<__u32>(__c);
  }
  // Divides by d, returns the remainder.
  _LIBCPP_HIDE_FROM_ABI inline __u32 __divmod_small(__u32 __d) {
    __u64 __r = 0;
    for (int __i = __n - 1; __i >= 0; --__i) {
      __u64 __cur = (__r << 32) | __l[__i];
      __l[__i]    = static_cast<__u32>(__cur / __d);
      __r       = __cur % __d;
    }
    __trim();
    return static_cast<__u32>(__r);
  }
  _LIBCPP_HIDE_FROM_ABI inline int __bit_length() const {
    if (__n == 0)
      return 0;
    return (__n - 1) * 32 + (32 - __builtin_clz(__l[__n - 1]));
  }
  _LIBCPP_HIDE_FROM_ABI inline bool __bit(int __i) const {
    int __w = __i >> 5;
    return __w < __n && ((__l[__w] >> (__i & 31)) & 1);
  }
  _LIBCPP_HIDE_FROM_ABI inline void __shl(int __bits) {
    if (__n == 0 || __bits == 0)
      return;
    int __w = __bits >> 5, __b = __bits & 31;
    if (__b == 0) {
      for (int __i = __n - 1; __i >= 0; --__i)
        __l[__i + __w] = __l[__i];
    } else {
      __l[__n + __w] = 0;
      for (int __i = __n - 1; __i >= 0; --__i) {
        __l[__i + __w + 1] |= __l[__i] >> (32 - __b);
        __l[__i + __w] = __l[__i] << __b;
      }
      // l[w + n] was written through the |= above on top of a zero.
    }
    for (int __i = 0; __i < __w; ++__i)
      __l[__i] = 0;
    __n += __w + (__b ? 1 : 0);
    __trim();
  }
  _LIBCPP_HIDE_FROM_ABI inline void __shr1() {
    __u32 __carry = 0;
    for (int __i = __n - 1; __i >= 0; --__i) {
      __u32 __nc = __l[__i] & 1;
      __l[__i]   = (__l[__i] >> 1) | (__carry << 31);
      __carry  = __nc;
    }
    __trim();
  }
  _LIBCPP_HIDE_FROM_ABI inline int __cmp(const __bignum& __o) const {
    if (__n != __o.__n)
      return __n < __o.__n ? -1 : 1;
    for (int __i = __n - 1; __i >= 0; --__i)
      if (__l[__i] != __o.__l[__i])
        return __l[__i] < __o.__l[__i] ? -1 : 1;
    return 0;
  }
  // *this -= o; requires *this >= o.
  _LIBCPP_HIDE_FROM_ABI inline void __sub(const __bignum& __o) {
    __u64 __borrow = 0;
    for (int __i = 0; __i < __n; ++__i) {
      __u64 __cur = static_cast<__u64>(__l[__i]) - __borrow - (__i < __o.__n ? __o.__l[__i] : 0);
      __l[__i]    = static_cast<__u32>(__cur);
      __borrow  = (__cur >> 63) & 1;
    }
    __trim();
  }
  _LIBCPP_HIDE_FROM_ABI inline void __mul_pow10(int __e) {
    while (__e >= 9) {
      __mul_small(1000000000u);
      __e -= 9;
    }
    static constexpr __u32 __p10[9] = {1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000};
    if (__e > 0)
      __mul_small(__p10[__e]);
  }
};

// ---------------------------------------------------------------------------------------------------------------------
// Binary128 helpers.
// ---------------------------------------------------------------------------------------------------------------------
inline constexpr int __mant_bits = 112; // stored fraction bits
inline constexpr int __exp_bias  = 16383;
inline constexpr int __min_exp2  = -16494; // exponent of the least significant bit of a subnormal

// An IEEE binary interchange format (or bfloat16) described by its precision (significand bits including the hidden
// bit), the largest exponent, the widths of the stored fields and the decimal magnitudes outside of which a value
// certainly overflows or underflows (a conservative filter in front of the exact computation).
struct __fp_format {
  int __p;
  int __emax;
  int __frac_bits;
  int __exp_bits;
  int __max_mag10;
  int __min_mag10;
};
inline constexpr __fp_format __binary128{113, 16383, 112, 15, 4934, -4970};
inline constexpr __fp_format __binary16{11, 15, 10, 5, 6, -9};
inline constexpr __fp_format __bfloat16{8, 127, 7, 8, 40, -46};
inline constexpr int __max_digits10_exp = 4932;

struct __decomposed {
  bool __neg;
  bool __is_inf;
  bool __is_nan;
  bool __is_zero;
  __u128 __mant; // integer significand (includes the hidden bit for normal numbers)
  int __exp2;  // value = mant * 2^exp2
  __u128 __raw_fraction;
  int __biased_exp;
};

_LIBCPP_HIDE_FROM_ABI inline __decomposed __decompose(__u128 __bits) {
  __decomposed __d{};
  __d.__neg          = static_cast<bool>(__bits >> 127);
  __d.__biased_exp   = static_cast<int>((__bits >> __mant_bits) & 0x7fff);
  __d.__raw_fraction = __bits & ((static_cast<__u128>(1) << __mant_bits) - 1);
  __d.__is_inf       = __d.__biased_exp == 0x7fff && __d.__raw_fraction == 0;
  __d.__is_nan       = __d.__biased_exp == 0x7fff && __d.__raw_fraction != 0;
  __d.__is_zero      = __d.__biased_exp == 0 && __d.__raw_fraction == 0;
  if (__d.__biased_exp == 0) {
    __d.__mant = __d.__raw_fraction;
    __d.__exp2 = __min_exp2;
  } else {
    __d.__mant = __d.__raw_fraction | (static_cast<__u128>(1) << __mant_bits);
    __d.__exp2 = __d.__biased_exp - __exp_bias - __mant_bits;
  }
  return __d;
}

// ---------------------------------------------------------------------------------------------------------------------
// Exact decimal expansion of a finite, non-zero binary128 value: the digits of the integer part followed by an
// unbounded stream of fractional digits (the expansion terminates, the stream then yields zeros).
// ---------------------------------------------------------------------------------------------------------------------
struct __expansion {
  static constexpr int __int_cap = 4940;
  char __int_digits[__int_cap];
  int __int_len = 0;
  int __int_pos = 0;
  __bignum<520> __frac;
  int __k = 0; // fractional denominator is 2^k (k == 0: no fractional part)
  int __pushed = -1;         // a digit that was read ahead and is to be returned first
  bool __lead_zero = false;  // return a virtual '0' before the integer digits

  _LIBCPP_HIDE_FROM_ABI inline void __init(__u128 __mant, int __exp2) {
    __int_len = 0;
    __int_pos = 0;
    __pushed = -1;
    __lead_zero = false;
    __frac.__zero();
    __k = 0;
    __bignum<520> __ip;
    if (__exp2 >= 0) {
      __ip.__set(__mant);
      __ip.__shl(__exp2);
    } else {
      int __kk = -__exp2;
      if (__kk >= 128) {
        __ip.__zero();
        __frac.__set(__mant);
        __k = __kk;
      } else {
        __ip.__set(__mant >> __kk);
        __u128 __f = __mant & ((static_cast<__u128>(1) << __kk) - 1);
        if (__f != 0) {
          __frac.__set(__f);
          __k = __kk;
        }
      }
    }
    // Integer digits.
    char __tmp[__int_cap];
    int __count = 0;
    while (!__ip.__is_zero()) {
      __u32 __chunk = __ip.__divmod_small(1000000000u);
      for (int __i = 0; __i < 9; ++__i) {
        __tmp[__count++] = static_cast<char>('0' + __chunk % 10);
        __chunk /= 10;
      }
    }
    while (__count > 0 && __tmp[__count - 1] == '0')
      --__count;
    __int_len = __count;
    for (int __i = 0; __i < __count; ++__i)
      __int_digits[__i] = __tmp[__count - 1 - __i];
  }

  _LIBCPP_HIDE_FROM_ABI inline int __next_fraction_digit() {
    if (__frac.__is_zero())
      return 0;
    __frac.__mul_small(10);
    int __idx = __k >> 5, __off = __k & 31;
    __u32 __lo   = __idx < __frac.__n ? __frac.__l[__idx] : 0;
    __u32 __hi   = (__idx + 1) < __frac.__n ? __frac.__l[__idx + 1] : 0;
    __u32 __word = __off == 0 ? __lo : ((__lo >> __off) | (__hi << (32 - __off)));
    int __digit = static_cast<int>(__word & 0xF);
    // Clear the bits at positions >= k.
    if (__idx < __frac.__n) {
      if (__off == 0)
        __frac.__l[__idx] = 0;
      else
        __frac.__l[__idx] &= (1u << __off) - 1;
      __frac.__n = __idx + 1;
      __frac.__trim();
    }
    return __digit;
  }

  // Next digit of the sequence "integer digits, fraction digits".
  _LIBCPP_HIDE_FROM_ABI inline int __next() {
    if (__pushed >= 0) {
      int __d  = __pushed;
      __pushed = -1;
      return __d;
    }
    if (__lead_zero) {
      __lead_zero = false;
      return 0;
    }
    if (__int_pos < __int_len)
      return __int_digits[__int_pos++] - '0';
    return __next_fraction_digit();
  }
  _LIBCPP_HIDE_FROM_ABI inline bool __rest_nonzero() const {
    if (__pushed > 0)
      return true;
    for (int __i = __int_pos; __i < __int_len; ++__i)
      if (__int_digits[__i] != '0')
        return true;
    return !__frac.__is_zero();
  }
};

// ---------------------------------------------------------------------------------------------------------------------
// Decimal / binary conversion of a digit string to a binary128 bit pattern (correct rounding, ties to even).
// ---------------------------------------------------------------------------------------------------------------------

// value = q * 2^e (+ a positive fraction below one unit of q when sticky is set); q != 0, q < 2^127.
// Returns false when the result overflows. Underflow to zero is reported through 'is_zero'.
_LIBCPP_HIDE_FROM_ABI inline bool __assemble(
    const __fp_format& __f, bool __neg, __u128 __q, int __e, bool __sticky, __u128& __bits, bool& __underflow_to_zero) {
  const int __sign_shift = __f.__frac_bits + __f.__exp_bits;
  __underflow_to_zero = false;
  int __bl = 0;
  for (__u128 __t = __q; __t; __t >>= 1)
    ++__bl;
  int __exp_unbiased = __e + __bl - 1; // exponent of the leading bit
  if (__exp_unbiased > __f.__emax) {
    return false;
  }
  int __keep = __f.__p;
  bool __subnormal = false;
  if (__exp_unbiased < 1 - __f.__emax) {
    __subnormal = true;
    __keep      = __f.__p - ((1 - __f.__emax) - __exp_unbiased);
  }
  __u128 __rounded;
  if (__keep >= __bl) {
    // No bits are dropped (exact).
    __rounded = __q << (__keep - __bl);
    // (the exponent adjustment is carried by 'keep' below)
    // A sticky fraction below the least significant kept bit never changes an exact value upwards unless it
    // exceeds half a unit, which is impossible when the kept width is not smaller than the value.
    int __shift = __keep - __bl;
    (void)__shift;
  } else {
    int __drop = __bl - __keep; // drop >= 1; may be as large as bl when keep <= 0
    if (__keep < 0) {
      // Far below the smallest subnormal: rounds to zero (the value is below half of the least significant bit).
      __underflow_to_zero = true;
      __bits              = static_cast<__u128>(__neg) << __sign_shift;
      return true;
    }
    __rounded       = __drop >= 128 ? 0 : (__q >> __drop);
    __u128 __rem      = __q & ((static_cast<__u128>(1) << __drop) - 1);
    __u128 __half     = static_cast<__u128>(1) << (__drop - 1);
    bool __up;
    if (__rem > __half)
      __up = true;
    else if (__rem < __half)
      __up = false;
    else
      __up = __sticky || (__rounded & 1);
    if (__up)
      ++__rounded;
  }
  __u128 __mag;
  if (__subnormal) {
    // 'rounded' counts units of the least significant subnormal bit; reaching 2^frac_bits is the smallest normal
    // number (exponent field 1).
    __mag = __rounded;
    if (__mag == 0)
      __underflow_to_zero = true;
  } else {
    int __biased = __exp_unbiased + __f.__emax;
    if (__rounded == (static_cast<__u128>(1) << __f.__p)) {
      __rounded >>= 1;
      ++__biased;
    }
    if (__biased >= (1 << __f.__exp_bits) - 1)
      return false;
    __mag = (static_cast<__u128>(__biased) << __f.__frac_bits) +
            (__rounded - (static_cast<__u128>(1) << __f.__frac_bits));
  }
  __bits = __mag | (static_cast<__u128>(__neg) << __sign_shift);
  return true;
}

// value = D * 10^e10 where D is the integer written by digs[0..n) (no leading zeros needed) and 'sticky' says that a
// further non-zero fraction below one unit of the last digit was dropped. Returns false on overflow.
_LIBCPP_HIDE_FROM_ABI inline bool __decimal_to_bits(
    const __fp_format& __f,
    bool __neg,
    const char* __digs,
    int __n,
    bool __sticky,
    long __e10,
    __u128& __bits,
    bool& __underflow_to_zero) {
  const int __sign_shift = __f.__frac_bits + __f.__exp_bits;
  __underflow_to_zero = false;
  int __first = 0;
  while (__first < __n && __digs[__first] == '0')
    ++__first;
  if (__first == __n) {
    __bits = static_cast<__u128>(__neg) << __sign_shift;
    return true;
  }
  __digs += __first;
  __n -= __first;
  if (__sticky) {
    // The dropped fraction is accounted for by an extra low digit '1' (enough digits are retained by the caller so that
    // this cannot hide or create a tie).
  }
  // Magnitude check.
  long __mag10 = static_cast<long>(__n) + __e10 + (__sticky ? 1 : 0);
  if (__mag10 > __f.__max_mag10)
    return false;
  if (__mag10 < __f.__min_mag10) {
    __underflow_to_zero = true;
    __bits              = static_cast<__u128>(__neg) << __sign_shift;
    return true;
  }
  __bignum<2100> __num, __den;
  __num.__zero();
  __den.__zero();
  __num.__n = 0;
  // Build the integer.
  int __pos = 0;
  while (__pos < __n) {
    int __take = __n - __pos < 9 ? __n - __pos : 9;
    __u32 __chunk = 0, __scale = 1;
    for (int __i = 0; __i < __take; ++__i) {
      __chunk = __chunk * 10 + static_cast<__u32>(__digs[__pos + __i] - '0');
      __scale *= 10;
    }
    if (__num.__n == 0) {
      __num.__set(__chunk);
    } else {
      __num.__mul_small(__scale);
      __num.__add_small(__chunk);
    }
    __pos += __take;
  }
  if (__sticky) {
    __num.__mul_small(10);
    __num.__add_small(1);
    --__e10;
  }
  __den.__set(1);
  if (__e10 >= 0)
    __num.__mul_pow10(static_cast<int>(__e10));
  else
    __den.__mul_pow10(static_cast<int>(-__e10));
  // quotient with at least 116 bits
  int __t     = __num.__bit_length() - __den.__bit_length();
  int __shift = 117 - __t; // value = (num / den) * 2^-shift after scaling
  if (__shift >= 0)
    __num.__shl(__shift);
  else
    __den.__shl(-__shift);
  // Now num / den has 117 or 118 bits.
  int __qbits = __num.__bit_length() - __den.__bit_length() + 1;
  // long division
  __u128 __q = 0;
  __bignum<2100> __d;
  __d = __den;
  __d.__shl(__qbits - 1);
  for (int __i = __qbits - 1; __i >= 0; --__i) {
    if (__num.__cmp(__d) >= 0) {
      __num.__sub(__d);
      __q |= static_cast<__u128>(1) << __i;
    }
    __d.__shr1();
  }
  bool __rem_nonzero = !__num.__is_zero();
  return __assemble(__f, __neg, __q, -__shift, __rem_nonzero, __bits, __underflow_to_zero);
}


// ---------------------------------------------------------------------------------------------------------------------
// Decimal output (to_chars).
// ---------------------------------------------------------------------------------------------------------------------
enum class __fmt_kind { __plain, __scientific, __fixed, __general, __hex };

struct __to_chars_out {
  char* __ptr;
  bool __too_large;
};

struct __rounding {
  bool __up;
  long __last_non9; // index of the last digit different from 9 (-1: none)
  long __last_nz;   // index of the last digit different from 0 (-1: none)
};

// Consumes n digits of a scratch expansion and decides whether the n-digit prefix is rounded up (ties to even).
_LIBCPP_HIDE_FROM_ABI inline __rounding __decide(__expansion& __e, long __n) {
  __rounding __r{false, -1, -1};
  int __last = 0;
  for (long __i = 0; __i < __n; ++__i) {
    int __d = __e.__next();
    if (__d != 9)
      __r.__last_non9 = __i;
    if (__d != 0)
      __r.__last_nz = __i;
    __last = __d;
  }
  int __rd = __e.__next();
  if (__rd > 5)
    __r.__up = true;
  else if (__rd < 5)
    __r.__up = false;
  else if (__e.__rest_nonzero())
    __r.__up = true;
  else
    __r.__up = (__last & 1) != 0;
  return __r;
}

// Produces the digits of the rounded n digit sequence one by one.
struct __digit_source {
  __expansion __e;
  __rounding __r;
  long __i = 0;
  bool __all_nines_up() const { return __r.__up && __r.__last_non9 == -1; }
  _LIBCPP_HIDE_FROM_ABI inline char __next() {
    int __d = __e.__next();
    int __c;
    if (!__r.__up)
      __c = __d;
    else if (__all_nines_up())
      __c = 0;
    else if (__i < __r.__last_non9)
      __c = __d;
    else if (__i == __r.__last_non9)
      __c = __d + 1;
    else
      __c = 0;
    ++__i;
    return static_cast<char>('0' + __c);
  }
};

_LIBCPP_HIDE_FROM_ABI inline int __dec_digits(unsigned __v) {
  int __n = 1;
  while (__v >= 10) {
    __v /= 10;
    ++__n;
  }
  return __n;
}

_LIBCPP_HIDE_FROM_ABI inline int __exponent_length(int __x) {
  if (__x < 0)
    __x = -__x;
  int __nd = __dec_digits(static_cast<unsigned>(__x));
  return 2 + (__nd < 2 ? 2 : __nd); // 'e', sign, at least two digits
}

_LIBCPP_HIDE_FROM_ABI inline char* __put_exponent(char* __p, int __x, char __ec) {
  *__p++ = __ec;
  if (__x < 0) {
    *__p++ = '-';
    __x = -__x;
  } else {
    *__p++ = '+';
  }
  int __nd = __dec_digits(static_cast<unsigned>(__x));
  char __tmp[12];
  for (int __i = __nd - 1; __i >= 0; --__i) {
    __tmp[__i] = static_cast<char>('0' + __x % 10);
    __x /= 10;
  }
  if (__nd < 2)
    *__p++ = '0';
  for (int __i = 0; __i < __nd; ++__i)
    *__p++ = __tmp[__i];
  return __p;
}

// Moves 'e' to its first significant digit and returns the decimal exponent of that digit.
_LIBCPP_HIDE_FROM_ABI inline int __skip_to_first_digit(__expansion& __e) {
  if (__e.__int_len > 0)
    return __e.__int_len - 1;
  int __z = 0;
  for (;;) {
    int __dg = __e.__next_fraction_digit();
    if (__dg != 0) {
      __e.__pushed = __dg;
      return -(__z + 1);
    }
    ++__z;
  }
}

// Number of trailing zeros of the rounded sequence of n digits (before an optional carry digit).
_LIBCPP_HIDE_FROM_ABI inline long __trailing_zeros(const __rounding& __r, long __n) {
  if (__r.__up) {
    if (__r.__last_non9 == -1)
      return __n; // "1" + n zeros (fixed) / the first digit becomes 1 and the rest zeros (scientific, see callers)
    return __n - 1 - __r.__last_non9;
  }
  return __r.__last_nz == -1 ? __n : __n - 1 - __r.__last_nz;
}

// Scientific notation with 'frac' digits after the point. With strip_zeros the trailing zeros of the fraction are
// dropped (the %g behaviour). 'x_out' receives the decimal exponent of the rounded value.
_LIBCPP_HIDE_FROM_ABI inline int __scientific_exponent(const __decomposed& __d, long __n) {
  __expansion __e;
  __e.__init(__d.__mant, __d.__exp2);
  int __x             = __skip_to_first_digit(__e);
  __expansion __scratch = __e;
  __rounding __r        = __decide(__scratch, __n);
  return __x + ((__r.__up && __r.__last_non9 == -1) ? 1 : 0);
}

_LIBCPP_HIDE_FROM_ABI inline __to_chars_out __format_scientific(char* __first, char* __last, const __decomposed& __d, long __frac, bool __strip_zeros) {
  __digit_source __src;
  __src.__e.__init(__d.__mant, __d.__exp2);
  int __x            = __skip_to_first_digit(__src.__e);
  long __n           = __frac + 1;
  __expansion __scratch = __src.__e;
  __src.__r            = __decide(__scratch, __n);
  bool __carry       = __src.__all_nines_up();
  int __xe           = __x + (__carry ? 1 : 0);
  long __out_frac    = __frac;
  if (__strip_zeros) {
    long __tz = __carry ? __n - 1 : __trailing_zeros(__src.__r, __n);
    if (__tz > __frac)
      __tz = __frac;
    __out_frac -= __tz;
  }
  long __len = (__d.__neg ? 1 : 0) + 1 + (__out_frac > 0 ? 1 + __out_frac : 0) + __exponent_length(__xe);
  if (__last - __first < __len)
    return {__last, true};
  char* __p = __first;
  if (__d.__neg)
    *__p++ = '-';
  char __c0 = __src.__next();
  *__p++    = __carry ? '1' : __c0;
  if (__out_frac > 0) {
    *__p++ = '.';
    for (long __i = 0; __i < __out_frac; ++__i)
      *__p++ = __src.__next();
  }
  __p = __put_exponent(__p, __xe, 'e');
  return {__p, false};
}

// Fixed notation with 'frac' digits after the point (frac >= 0); trailing zeros of the fraction are dropped with
// strip_zeros (and then the point if nothing is left).
_LIBCPP_HIDE_FROM_ABI inline __to_chars_out __format_fixed(char* __first, char* __last, const __decomposed& __d, long __frac, bool __strip_zeros) {
  __digit_source __src;
  __src.__e.__init(__d.__mant, __d.__exp2);
  long __n_int = __src.__e.__int_len;
  if (__n_int == 0) {
    __src.__e.__lead_zero = true;
    __n_int           = 1;
  }
  long __n            = __n_int + __frac;
  __expansion __scratch = __src.__e;
  __src.__r             = __decide(__scratch, __n);
  bool __carry        = __src.__all_nines_up();
  long __out_frac     = __frac;
  if (__strip_zeros) {
    long __tz = __trailing_zeros(__src.__r, __n);
    if (__tz > __frac)
      __tz = __frac;
    __out_frac -= __tz;
  }
  long __len = (__d.__neg ? 1 : 0) + __n_int + (__carry ? 1 : 0) + (__out_frac > 0 ? 1 + __out_frac : 0);
  if (__last - __first < __len)
    return {__last, true};
  char* __p = __first;
  if (__d.__neg)
    *__p++ = '-';
  if (__carry)
    *__p++ = '1';
  for (long __i = 0; __i < __n_int; ++__i)
    *__p++ = __src.__next();
  if (__out_frac > 0) {
    *__p++ = '.';
    for (long __i = 0; __i < __out_frac; ++__i)
      *__p++ = __src.__next();
  }
  return {__p, false};
}


// General notation with a precision (the %g behaviour: trailing zeros are removed).
_LIBCPP_HIDE_FROM_ABI inline __to_chars_out __format_general(char* __first, char* __last, const __decomposed& __d, long __precision) {
  long __P = __precision == 0 ? 1 : __precision;
  int __x  = __scientific_exponent(__d, __P);
  if (__P > __x && __x >= -4)
    return __format_fixed(__first, __last, __d, __P - 1 - __x, true);
  return __format_scientific(__first, __last, __d, __P - 1, true);
}

// ---------------------------------------------------------------------------------------------------------------------
// Shortest round-trip digits.
// ---------------------------------------------------------------------------------------------------------------------
inline constexpr int __max_sig_digits = 36;

struct __shortest_digits {
  char __digs[__max_sig_digits + 2];
  int __n;
  int __x; // decimal exponent of the first digit
};

_LIBCPP_HIDE_FROM_ABI inline bool __digits_round_trip(const char* __digs, int __n, int __x, __u128 __magnitude_bits) {
  __u128 __bits;
  bool __uz;
  if (!__decimal_to_bits(__binary128, false, __digs, __n, false, static_cast<long>(__x) - (__n - 1), __bits, __uz))
    return false;
  return !__uz && __bits == __magnitude_bits;
}

_LIBCPP_HIDE_FROM_ABI inline __shortest_digits __find_shortest(const __decomposed& __d, __u128 __magnitude_bits) {
  __shortest_digits __out{};
  __expansion __base;
  __base.__init(__d.__mant, __d.__exp2);
  int __x0 = __skip_to_first_digit(__base);
  for (int __n = 1; __n <= __max_sig_digits; ++__n) {
    __expansion __scratch = __base;
    __rounding __r        = __decide(__scratch, __n);
    // truncated digits
    char __t[__max_sig_digits + 2];
    {
      __expansion __w = __base;
      for (int __i = 0; __i < __n; ++__i)
        __t[__i] = static_cast<char>('0' + __w.__next());
    }
    // rounded up candidate
    char __u[__max_sig_digits + 2];
    int __ux = __x0;
    {
      for (int __i = 0; __i < __n; ++__i)
        __u[__i] = __t[__i];
      int __i = __n - 1;
      while (__i >= 0 && __u[__i] == '9') {
        __u[__i] = '0';
        --__i;
      }
      if (__i >= 0) {
        ++__u[__i];
      } else {
        __u[0] = '1';
        for (int __j = 1; __j < __n; ++__j)
          __u[__j] = '0';
        __ux = __x0 + 1;
      }
    }
    const char* __first_c  = __r.__up ? __u : __t;
    int __first_x          = __r.__up ? __ux : __x0;
    const char* __second_c = __r.__up ? __t : __u;
    int __second_x         = __r.__up ? __x0 : __ux;
    const char* __pick     = nullptr;
    int __pick_x           = 0;
    if (__digits_round_trip(__first_c, __n, __first_x, __magnitude_bits)) {
      __pick = __first_c;
      __pick_x = __first_x;
    } else if (__digits_round_trip(__second_c, __n, __second_x, __magnitude_bits)) {
      __pick = __second_c;
      __pick_x = __second_x;
    }
    if (__pick) {
      for (int __i = 0; __i < __n; ++__i)
        __out.__digs[__i] = __pick[__i];
      __out.__n = __n;
      __out.__x = __pick_x;
      return __out;
    }
  }
  // Not reached: 36 digits always round-trip.
  __out.__n = 0;
  return __out;
}

// Prints shortest digits in the given notation. fixed with a non-negative decimal exponent of the last digit prints the
// exact value of the number (as the double implementation does); this is done by the caller.
_LIBCPP_HIDE_FROM_ABI inline __to_chars_out __print_shortest_scientific(char* __first, char* __last, bool __neg, const __shortest_digits& __s) {
  long __len = (__neg ? 1 : 0) + 1 + (__s.__n > 1 ? __s.__n : 0) + __exponent_length(__s.__x);
  if (__last - __first < __len)
    return {__last, true};
  char* __p = __first;
  if (__neg)
    *__p++ = '-';
  *__p++ = __s.__digs[0];
  if (__s.__n > 1) {
    *__p++ = '.';
    for (int __i = 1; __i < __s.__n; ++__i)
      *__p++ = __s.__digs[__i];
  }
  __p = __put_exponent(__p, __s.__x, 'e');
  return {__p, false};
}

_LIBCPP_HIDE_FROM_ABI inline __to_chars_out __print_shortest_fixed(char* __first, char* __last, bool __neg, const __shortest_digits& __s) {
  // Only used when the last digit has a negative decimal exponent, or when the digits are all integral and the
  // total length is small; the caller handles huge integers.
  int __e10 = __s.__x - (__s.__n - 1);
  long __len;
  if (__e10 >= 0)
    __len = (__neg ? 1 : 0) + __s.__n + __e10;
  else if (__s.__x >= 0)
    __len = (__neg ? 1 : 0) + __s.__n + 1;
  else
    __len = (__neg ? 1 : 0) + 2 + (-__s.__x - 1) + __s.__n;
  if (__last - __first < __len)
    return {__last, true};
  char* __p = __first;
  if (__neg)
    *__p++ = '-';
  if (__e10 >= 0) {
    for (int __i = 0; __i < __s.__n; ++__i)
      *__p++ = __s.__digs[__i];
    for (int __i = 0; __i < __e10; ++__i)
      *__p++ = '0';
  } else if (__s.__x >= 0) {
    for (int __i = 0; __i < __s.__n; ++__i) {
      if (__i == __s.__x + 1)
        *__p++ = '.';
      *__p++ = __s.__digs[__i];
    }
  } else {
    *__p++ = '0';
    *__p++ = '.';
    for (int __i = 0; __i < -__s.__x - 1; ++__i)
      *__p++ = '0';
    for (int __i = 0; __i < __s.__n; ++__i)
      *__p++ = __s.__digs[__i];
  }
  return {__p, false};
}

// ---------------------------------------------------------------------------------------------------------------------
// Hexadecimal output.
// ---------------------------------------------------------------------------------------------------------------------
_LIBCPP_HIDE_FROM_ABI inline __to_chars_out __format_hex(char* __first, char* __last, const __decomposed& __d, bool __has_precision, long __precision) {
  static constexpr char __hexd[] = "0123456789abcdef";
  __u128 __mant = __d.__raw_fraction;
  int __lead  = __d.__biased_exp == 0 ? 0 : 1;
  int __exp2  = __d.__is_zero ? 0 : (__d.__biased_exp == 0 ? 1 - __exp_bias : __d.__biased_exp - __exp_bias);
  // fraction as 28 nibbles
  char __frac_digits[28];
  int __top = __lead;
  long __nfrac;
  if (__has_precision && __precision < 28) {
    int __shift = 4 * (28 - static_cast<int>(__precision));
    __u128 __m    = (static_cast<__u128>(__lead) << __mant_bits) | __mant;
    __u128 __q    = __m >> __shift;
    __u128 __rem  = __m & ((static_cast<__u128>(1) << __shift) - 1);
    __u128 __half = static_cast<__u128>(1) << (__shift - 1);
    if (__rem > __half || (__rem == __half && (__q & 1)))
      ++__q;
    __top = static_cast<int>(__q >> (4 * __precision));
    __u128 __fm = __q & ((static_cast<__u128>(1) << (4 * __precision)) - 1);
    for (long __i = 0; __i < __precision; ++__i)
      __frac_digits[__i] = __hexd[static_cast<int>((__fm >> (4 * (__precision - 1 - __i))) & 0xf)];
    __nfrac = __precision;
  } else {
    for (int __i = 0; __i < 28; ++__i)
      __frac_digits[__i] = __hexd[static_cast<int>((__mant >> (4 * (27 - __i))) & 0xf)];
    __nfrac = 28;
    if (!__has_precision) {
      while (__nfrac > 0 && __frac_digits[__nfrac - 1] == '0')
        --__nfrac;
    }
  }
  long __pad = __has_precision && __precision > 28 ? __precision - 28 : 0;
  long __out_frac = __nfrac + __pad;
  int __ex = __exp2;
  int __ndig = __dec_digits(static_cast<unsigned>(__ex < 0 ? -__ex : __ex));
  long __len = (__d.__neg ? 1 : 0) + 1 + (__out_frac > 0 ? 1 + __out_frac : 0) + 2 + __ndig;
  if (__last - __first < __len)
    return {__last, true};
  char* __p = __first;
  if (__d.__neg)
    *__p++ = '-';
  *__p++ = __hexd[__top];
  if (__out_frac > 0) {
    *__p++ = '.';
    for (long __i = 0; __i < __nfrac; ++__i)
      *__p++ = __frac_digits[__i];
    for (long __i = 0; __i < __pad; ++__i)
      *__p++ = '0';
  }
  *__p++ = 'p';
  *__p++ = __ex < 0 ? '-' : '+';
  unsigned __ax = static_cast<unsigned>(__ex < 0 ? -__ex : __ex);
  char __tmp[8];
  for (int __i = __ndig - 1; __i >= 0; --__i) {
    __tmp[__i] = static_cast<char>('0' + __ax % 10);
    __ax /= 10;
  }
  for (int __i = 0; __i < __ndig; ++__i)
    *__p++ = __tmp[__i];
  return {__p, false};
}

// ---------------------------------------------------------------------------------------------------------------------
// to_chars entry point.
// ---------------------------------------------------------------------------------------------------------------------
_LIBCPP_HIDE_FROM_ABI inline __to_chars_out __to_chars(char* __first, char* __last, __u128 __bits, __fmt_kind __kind, bool __has_precision, int __precision) {
  __decomposed __d = __decompose(__bits);
  if (__d.__is_inf || __d.__is_nan) {
    const char* __str;
    long __len;
    if (__d.__is_inf) {
      __str = "inf";
      __len = 3;
    } else if (__d.__neg && __d.__raw_fraction == (static_cast<__u128>(1) << 111)) {
      __str = "nan(ind)";
      __len = 8;
    } else if ((__d.__raw_fraction >> 111) & 1) {
      __str = "nan";
      __len = 3;
    } else {
      __str = "nan(snan)";
      __len = 9;
    }
    long __total = (__d.__neg ? 1 : 0) + __len;
    if (__last - __first < __total)
      return {__last, true};
    char* __p = __first;
    if (__d.__neg)
      *__p++ = '-';
    for (long __i = 0; __i < __len; ++__i)
      *__p++ = __str[__i];
    return {__p, false};
  }
  if (__has_precision && __precision < 0)
    __precision = 6;
  if (__kind == __fmt_kind::__hex)
    return __format_hex(__first, __last, __d, __has_precision, __precision);
  if (__d.__is_zero) {
    long __len = (__d.__neg ? 1 : 0) + 1;
    int __ndig = 0;
    switch (__kind) {
    case __fmt_kind::__plain:
    case __fmt_kind::__general:
      break;
    case __fmt_kind::__scientific:
      __len += (__has_precision && __precision > 0 ? 1 + __precision : 0) + 4; // e+00
      __ndig = 1;
      break;
    case __fmt_kind::__fixed:
      __len += (__has_precision && __precision > 0 ? 1 + __precision : 0);
      break;
    default:
      break;
    }
    if (__kind == __fmt_kind::__general && __has_precision) {
      // %g removes the trailing zeros, "0" remains.
    }
    (void)__ndig;
    if (__last - __first < __len)
      return {__last, true};
    char* __p = __first;
    if (__d.__neg)
      *__p++ = '-';
    *__p++ = '0';
    if ((__kind == __fmt_kind::__scientific || __kind == __fmt_kind::__fixed) && __has_precision && __precision > 0) {
      *__p++ = '.';
      for (long __i = 0; __i < __precision; ++__i)
        *__p++ = '0';
    }
    if (__kind == __fmt_kind::__scientific) {
      *__p++ = 'e';
      *__p++ = '+';
      *__p++ = '0';
      *__p++ = '0';
    }
    return {__p, false};
  }
  if (__has_precision) {
    switch (__kind) {
    case __fmt_kind::__scientific:
      return __format_scientific(__first, __last, __d, __precision, false);
    case __fmt_kind::__fixed:
      return __format_fixed(__first, __last, __d, __precision, false);
    case __fmt_kind::__general:
      return __format_general(__first, __last, __d, __precision);
    default:
      break;
    }
  }
  // shortest
  __u128 __mag_bits = __bits & ~(static_cast<__u128>(1) << 127);
  __shortest_digits __s = __find_shortest(__d, __mag_bits);
  int __e10 = __s.__x - (__s.__n - 1);
  __fmt_kind __use = __kind;
  if (__kind == __fmt_kind::__plain) {
    long __lower, __upper;
    if (__s.__n == 1) {
      __lower = -3;
      __upper = 4;
    } else {
      __lower = -(static_cast<long>(__s.__n) + 3);
      __upper = 5;
    }
    __use = (__lower <= __e10 && __e10 <= __upper) ? __fmt_kind::__fixed : __fmt_kind::__scientific;
  } else if (__kind == __fmt_kind::__general) {
    __use = (-4 <= __s.__x && __s.__x < 6) ? __fmt_kind::__fixed : __fmt_kind::__scientific;
  }
  if (__use == __fmt_kind::__scientific)
    return __print_shortest_scientific(__first, __last, __d.__neg, __s);
  // fixed
  if (__e10 >= 0 && __kind == __fmt_kind::__fixed)
    return __format_fixed(__first, __last, __d, 0, false); // exact integer value
  return __print_shortest_fixed(__first, __last, __d.__neg, __s);
}


// ---------------------------------------------------------------------------------------------------------------------
// from_chars.
// ---------------------------------------------------------------------------------------------------------------------
enum class __fc_status { __ok, __invalid, __out_of_range };

struct __from_chars_out {
  const char* __ptr;
  __fc_status __status;
  __u128 __bits;
};

_LIBCPP_HIDE_FROM_ABI inline char __ascii_lower(char __c) { return (__c >= 'A' && __c <= 'Z') ? static_cast<char>(__c - 'A' + 'a') : __c; }
_LIBCPP_HIDE_FROM_ABI inline bool __ascii_digit(char __c) { return __c >= '0' && __c <= '9'; }
_LIBCPP_HIDE_FROM_ABI inline bool __ascii_alnum(char __c) {
  return __ascii_digit(__c) || (__c >= 'a' && __c <= 'z') || (__c >= 'A' && __c <= 'Z');
}
_LIBCPP_HIDE_FROM_ABI inline int __hex_value(char __c) {
  if (__c >= '0' && __c <= '9')
    return __c - '0';
  __c = __ascii_lower(__c);
  if (__c >= 'a' && __c <= 'f')
    return __c - 'a' + 10;
  return -1;
}

inline constexpr int __max_retained_digits = 11700;

struct __exponent_parse {
  const char* __ptr;
  long __value;
  bool __present;
};

// [e|p][+|-]digits; the marker is matched case-insensitively. Huge values saturate.
_LIBCPP_HIDE_FROM_ABI inline __exponent_parse __parse_exponent(const char* __p, const char* __last, char __marker) {
  if (__p + 1 < __last && __ascii_lower(*__p) == __marker && !(*(__p + 1) == ' ' || (*(__p + 1) >= '\t' && *(__p + 1) <= '\r'))) {
    const char* __q = __p + 1;
    bool __neg      = false;
    if (__q < __last && (*__q == '+' || *__q == '-')) {
      __neg = *__q == '-';
      ++__q;
    }
    if (__q < __last && __ascii_digit(*__q)) {
      long __v = 0;
      while (__q < __last && __ascii_digit(*__q)) {
        if (__v < 100000000)
          __v = __v * 10 + (*__q - '0');
        ++__q;
      }
      return {__q, __neg ? -__v : __v, true};
    }
  }
  return {__p, 0, false};
}

_LIBCPP_HIDE_FROM_ABI inline __from_chars_out __make_inf(const __fp_format& __f, bool __neg, const char* __ptr) {
  return {__ptr,
          __fc_status::__ok,
          (((static_cast<__u128>(1) << __f.__exp_bits) - 1) << __f.__frac_bits) |
              (static_cast<__u128>(__neg) << (__f.__frac_bits + __f.__exp_bits))};
}

_LIBCPP_HIDE_FROM_ABI inline __from_chars_out
__from_chars(const char* __first, const char* __last, __fmt_kind __kind, const __fp_format& __f = __binary128) {
  const int __sign_shift = __f.__frac_bits + __f.__exp_bits;
  if (__first == __last)
    return {__first, __fc_status::__invalid, 0};
  const char* __p = __first;
  bool __neg      = *__p == '-';
  if (__neg) {
    ++__p;
    if (__p == __last)
      return {__first, __fc_status::__invalid, 0};
  }
  char __c0 = __ascii_lower(*__p);
  if (__c0 == 'i') {
    const char* __q = __p + 1;
    if (__last - __q < 2 || __ascii_lower(__q[0]) != 'n' || __ascii_lower(__q[1]) != 'f')
      return {__first, __fc_status::__invalid, 0};
    __q += 2;
    if (__last - __q >= 5 && __ascii_lower(__q[0]) == 'i' && __ascii_lower(__q[1]) == 'n' && __ascii_lower(__q[2]) == 'i' &&
        __ascii_lower(__q[3]) == 't' && __ascii_lower(__q[4]) == 'y')
      __q += 5;
    return __make_inf(__f, __neg, __q);
  }
  if (__c0 == 'n') {
    const char* __q = __p + 1;
    if (__last - __q < 2 || __ascii_lower(__q[0]) != 'a' || __ascii_lower(__q[1]) != 'n')
      return {__first, __fc_status::__invalid, 0};
    __q += 2;
    if (__last - __q >= 2 && __q[0] == '(') {
      long __off = 1;
      do {
        if (__q[__off] == ')') {
          __q += __off + 1;
          break;
        }
        if (__q[__off] != '_' && !__ascii_alnum(__q[__off]))
          break;
        ++__off;
      } while (__q + __off != __last);
    }
    return {__q, __fc_status::__ok,
            (((static_cast<__u128>(1) << __f.__exp_bits) - 1) << __f.__frac_bits) |
                (static_cast<__u128>(1) << (__f.__frac_bits - 1)) | (static_cast<__u128>(__neg) << __sign_shift)};
  }

  if (__kind == __fmt_kind::__hex) {
    // hexadecimal digits with an optional point and an optional binary exponent.
    __u128 __h = 0;
    int __kept_bits = 0; // significant bits accumulated in h (leading zeros skipped)
    bool __sticky   = false;
    bool __valid    = false;
    bool __fraction = false;
    long __exp2     = 0;
    const char* __q = __p;
    for (; __q < __last; ++__q) {
      int __hv = __hex_value(*__q);
      if (__hv >= 0) {
        __valid = true;
        if (__h == 0 && __hv == 0) {
          if (__fraction)
            __exp2 -= 4;
        } else if (__kept_bits + 4 <= 124) {
          __h = (__h << 4) | static_cast<__u128>(__hv);
          __kept_bits += 4;
          if (__fraction)
            __exp2 -= 4;
        } else {
          if (__hv != 0)
            __sticky = true;
          if (!__fraction)
            __exp2 += 4;
        }
      } else if (*__q == '.') {
        if (__fraction)
          break;
        __fraction = true;
      } else {
        break;
      }
    }
    if (!__valid)
      return {__first, __fc_status::__invalid, 0};
    __exponent_parse __ep = __parse_exponent(__q, __last, 'p');
    __q                 = __ep.__ptr;
    __exp2 += __ep.__value;
    if (__h == 0)
      return {__q, __fc_status::__ok, static_cast<__u128>(__neg) << __sign_shift};
    int __bl = 0;
    for (__u128 __t = __h; __t; __t >>= 1)
      ++__bl;
    // normalise to 117 bits
    if (__bl < 117) {
      __h <<= (117 - __bl);
      __exp2 -= (117 - __bl);
    }
    __u128 __bits;
    bool __uz;
    if (!__assemble(__f, __neg, __h, static_cast<int>(__exp2 < -100000 ? -100000 : (__exp2 > 100000 ? 100000 : __exp2)), __sticky, __bits, __uz))
      return {__q, __fc_status::__out_of_range, 0};
    if (__uz)
      return {__q, __fc_status::__out_of_range, 0};
    return {__q, __fc_status::__ok, __bits};
  }

  // decimal
  static_assert(__max_retained_digits > 0, "");
  char __digs[__max_retained_digits];
  int __count   = 0;
  long __e10    = 0;
  bool __sticky = false;
  bool __valid  = false;
  bool __fraction = false;
  const char* __q = __p;
  for (; __q < __last; ++__q) {
    char __c = *__q;
    if (__ascii_digit(__c)) {
      __valid = true;
      int __dv = __c - '0';
      if (__count == 0 && __dv == 0) {
        if (__fraction)
          --__e10;
      } else if (__count < __max_retained_digits) {
        __digs[__count++] = __c;
        if (__fraction)
          --__e10;
      } else {
        if (__dv != 0)
          __sticky = true;
        if (!__fraction)
          ++__e10;
      }
    } else if (__c == '.') {
      if (__fraction)
        break;
      __fraction = true;
    } else {
      break;
    }
  }
  if (!__valid)
    return {__first, __fc_status::__invalid, 0};
  long __exp10 = 0;
  if (__kind != __fmt_kind::__fixed) {
    __exponent_parse __ep = __parse_exponent(__q, __last, 'e');
    if (__kind == __fmt_kind::__scientific && !__ep.__present)
      return {__first, __fc_status::__invalid, 0};
    __q     = __ep.__ptr;
    __exp10 = __ep.__value;
  }
  if (__count == 0)
    return {__q, __fc_status::__ok, static_cast<__u128>(__neg) << __sign_shift};
  __u128 __bits;
  bool __uz;
  if (!__decimal_to_bits(__f, __neg, __digs, __count, __sticky, __e10 + __exp10, __bits, __uz))
    return {__q, __fc_status::__out_of_range, 0};
  if (__uz)
    return {__q, __fc_status::__out_of_range, 0};
  return {__q, __fc_status::__ok, __bits};
}


} // namespace __f128

#endif // _LIBCPP_STD_VER >= 23 && defined(__SIZEOF_INT128__)

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___CHARCONV_FLOAT128_H
