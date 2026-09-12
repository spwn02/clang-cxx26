//===----------------------------------------------------------------------===//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___RANDOM_PHILOX_ENGINE_H
#define _LIBCPP___RANDOM_PHILOX_ENGINE_H

#include <__config>
#include <__cstddef/size_t.h>
#include <__random/is_seed_sequence.h>
#include <__type_traits/enable_if.h>
#include <__type_traits/is_unsigned.h>
#include <array>
#include <cstdint>
#include <iosfwd>
#include <limits>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_PUSH_MACROS
#include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

template <class _UInt, size_t _Np, _UInt... _Cp>
struct __philox_constants;
template <class _UInt, _UInt _M0, _UInt _C0>
struct __philox_constants<_UInt, 2, _M0, _C0> {
  static inline _LIBCPP_CONSTEXPR array<_UInt, 1> __multipliers  = {_M0};
  static inline _LIBCPP_CONSTEXPR array<_UInt, 1> __round_consts = {_C0};
};
template <class _UInt, _UInt _M0, _UInt _C0, _UInt _M1, _UInt _C1>
struct __philox_constants<_UInt, 4, _M0, _C0, _M1, _C1> {
  static inline _LIBCPP_CONSTEXPR array<_UInt, 2> __multipliers  = {_M0, _M1};
  static inline _LIBCPP_CONSTEXPR array<_UInt, 2> __round_consts = {_C0, _C1};
};

template <class _UIntType, size_t __w, size_t __n, size_t __r, _UIntType... __consts>
class philox_engine;
template <class _CharT, class _Traits, class _UIntType, size_t __w, size_t __n, size_t __r, _UIntType... __consts>
_LIBCPP_HIDE_FROM_ABI basic_ostream<_CharT, _Traits>&
operator<<(basic_ostream<_CharT, _Traits>&, const philox_engine<_UIntType, __w, __n, __r, __consts...>&);
template <class _CharT, class _Traits, class _UIntType, size_t __w, size_t __n, size_t __r, _UIntType... __consts>
_LIBCPP_HIDE_FROM_ABI basic_istream<_CharT, _Traits>&
operator>>(basic_istream<_CharT, _Traits>&, philox_engine<_UIntType, __w, __n, __r, __consts...>&);

template <class _UIntType, size_t __w, size_t __n, size_t __r, _UIntType... __consts>
class philox_engine {
public:
  using result_type = _UIntType;

private:
  static constexpr size_t __array_size = __n / 2;
  using __constants_type               = __philox_constants<_UIntType, __n, __consts...>;
  array<result_type, __array_size> __key_;
  array<result_type, __n> __counter_;
  array<result_type, __n> __output_;
  size_t __index_;
  static_assert(is_unsigned<_UIntType>::value, "philox_engine requires an unsigned result type");
  static_assert(sizeof...(__consts) == __n, "philox_engine invalid constants");
  static_assert(__n == 2 || __n == 4, "philox_engine invalid word count");
  static_assert(__r > 0, "philox_engine invalid round count");
  static_assert(__w > 0 && __w <= numeric_limits<result_type>::digits, "philox_engine invalid word size");
  static_assert(__w <= 64, "philox_engine word sizes greater than 64 are not supported");
  static inline _LIBCPP_CONSTEXPR const result_type __max =
      __w == numeric_limits<result_type>::digits ? result_type(~0) : (result_type(1) << __w) - result_type(1);

  _LIBCPP_HIDE_FROM_ABI static result_type __mulhi(result_type __a, result_type __b) {
    __a &= __max;
    __b &= __max;
#if _LIBCPP_HAS_INT128
    return static_cast<result_type>((static_cast<__uint128_t>(__a) * static_cast<__uint128_t>(__b)) >> __w);
#else
    const uint64_t __a0 = static_cast<uint64_t>(__a) & 0xffffffffu, __a1 = static_cast<uint64_t>(__a) >> 32;
    const uint64_t __b0 = static_cast<uint64_t>(__b) & 0xffffffffu, __b1 = static_cast<uint64_t>(__b) >> 32;
    const uint64_t __w0 = __a0 * __b0, __t = __a1 * __b0 + (__w0 >> 32), __w1 = __t & 0xffffffffu;
    const uint64_t __u = __a0 * __b1 + __w1;
    return static_cast<result_type>(__w == 64 ? __a1 * __b1 + (__t >> 32) + (__u >> 32)
                                              : (static_cast<uint64_t>(__a) * static_cast<uint64_t>(__b)) >> __w);
#endif
  }
  _LIBCPP_HIDE_FROM_ABI void __increment_counter() {
    for (size_t __j = 0; __j != __n; ++__j) {
      if (__counter_[__j] != __max) {
        ++__counter_[__j];
        return;
      }
      __counter_[__j] = 0;
    }
  }
  _LIBCPP_HIDE_FROM_ABI void __add_to_counter(unsigned long long __z) {
    for (size_t __j = 0; __j < __n && __j < 64 / __w; ++__j) {
      const result_type __add = static_cast<result_type>(__z >> (__j * __w)) & __max;
      if (__add == 0)
        continue;
      const bool __carry = __counter_[__j] > __max - __add;
      __counter_[__j]    = (__counter_[__j] + __add) & __max;
      if (!__carry)
        return;
      for (++__j; __j != __n; ++__j) {
        if (__counter_[__j] != __max) {
          ++__counter_[__j];
          return;
        }
        __counter_[__j] = 0;
      }
      return;
    }
  }
  _LIBCPP_HIDE_FROM_ABI array<result_type, __n> __philox(array<result_type, __n> __x) const {
    for (size_t __q = 0; __q != __r; ++__q) {
      array<result_type, __n> __v;
      if (__n == 2) {
        __v[0] = __x[0];
        __v[1] = __x[1];
      } else { // LWG 4134's word order matches Random123's reference implementation.
        __v[0] = __x[2];
        __v[1] = __x[1];
        __v[2] = __x[0];
        __v[3] = __x[3];
      }
      for (size_t __k = 0; __k != __array_size; ++__k) {
        const result_type __lo = (__v[2 * __k] * multipliers[__k]) & __max;
        const result_type __hi = __mulhi(__v[2 * __k], multipliers[__k]);
        __x[2 * __k] =
            (__hi ^ (__key_[__k] + static_cast<result_type>(__q) * round_consts[__k]) ^ __v[2 * __k + 1]) & __max;
        __x[2 * __k + 1] = __lo;
      }
    }
    return __x;
  }
  _LIBCPP_HIDE_FROM_ABI void __generate() {
    __output_ = __philox(__counter_);
    __increment_counter();
    __index_ = 0;
  }
  _LIBCPP_HIDE_FROM_ABI void __reconstruct_output() {
    if (__index_ == __n - 1)
      return;
    array<result_type, __n> __previous = __counter_;
    for (size_t __j = 0; __j != __n; ++__j) {
      if (__previous[__j] != 0) {
        --__previous[__j];
        break;
      }
      __previous[__j] = __max;
    }
    __output_ = __philox(__previous);
  }

public:
  static inline _LIBCPP_CONSTEXPR const size_t word_size = __w, word_count = __n, round_count = __r;
  static inline _LIBCPP_CONSTEXPR const array<result_type, __array_size> multipliers = __constants_type::__multipliers;
  static inline _LIBCPP_CONSTEXPR const array<result_type, __array_size> round_consts =
      __constants_type::__round_consts;
  _LIBCPP_HIDE_FROM_ABI static _LIBCPP_CONSTEXPR result_type min() { return 0; }
  _LIBCPP_HIDE_FROM_ABI static _LIBCPP_CONSTEXPR result_type max() { return __max; }
  static inline _LIBCPP_CONSTEXPR const result_type default_seed = 20111115u;
  _LIBCPP_HIDE_FROM_ABI philox_engine() : philox_engine(default_seed) {}
  _LIBCPP_HIDE_FROM_ABI explicit philox_engine(result_type __value) { seed(__value); }
  template <class _Sseq, __enable_if_t<__is_seed_sequence<_Sseq, philox_engine>::value, int> = 0>
  _LIBCPP_HIDE_FROM_ABI explicit philox_engine(_Sseq& __q) {
    seed(__q);
  }
  _LIBCPP_HIDE_FROM_ABI void seed(result_type __value = default_seed) {
    __key_.fill(0);
    __key_[0] = __value & __max;
    __counter_.fill(0);
    __index_ = __n - 1;
  }
  template <class _Sseq, __enable_if_t<__is_seed_sequence<_Sseq, philox_engine>::value, int> = 0>
  _LIBCPP_HIDE_FROM_ABI void seed(_Sseq& __q) {
    constexpr size_t __p = (__w + 31) / 32;
    uint32_t __a[__array_size * __p];
    __q.generate(__a, __a + __array_size * __p);
    for (size_t __k = 0; __k != __array_size; ++__k) {
      __key_[__k] = 0;
      for (size_t __j = 0; __j != __p; ++__j)
        __key_[__k] |= static_cast<result_type>(__a[__k * __p + __j]) << (32 * __j);
      __key_[__k] &= __max;
    }
    __counter_.fill(0);
    __index_ = __n - 1;
  }
  _LIBCPP_HIDE_FROM_ABI void set_counter(const array<result_type, __n>& __c) {
    for (size_t __j = 0; __j != __n; ++__j)
      __counter_[__j] = __c[__n - 1 - __j] & __max;
    __index_ = __n - 1;
  }
  _LIBCPP_HIDE_FROM_ABI result_type operator()() {
    if (++__index_ == __n)
      __generate();
    return __output_[__index_];
  }
  _LIBCPP_HIDE_FROM_ABI void discard(unsigned long long __z) {
    const size_t __available = __n - 1 - __index_;
    if (__z <= __available) {
      __index_ += static_cast<size_t>(__z);
      return;
    }
    __z -= __available;
    __add_to_counter(__z / __n);
    __z %= __n;
    if (__z != 0) {
      __generate();
      __index_ = static_cast<size_t>(__z - 1);
    }
  }
  template <class _UInt, size_t _Wp, size_t _Np, size_t _Rp, _UInt... _Cp>
  friend bool
  operator==(const philox_engine<_UInt, _Wp, _Np, _Rp, _Cp...>&, const philox_engine<_UInt, _Wp, _Np, _Rp, _Cp...>&);
  template <class _CharT, class _Traits, class _UInt, size_t _Wp, size_t _Np, size_t _Rp, _UInt... _Cp>
  friend basic_ostream<_CharT, _Traits>&
  operator<<(basic_ostream<_CharT, _Traits>&, const philox_engine<_UInt, _Wp, _Np, _Rp, _Cp...>&);
  template <class _CharT, class _Traits, class _UInt, size_t _Wp, size_t _Np, size_t _Rp, _UInt... _Cp>
  friend basic_istream<_CharT, _Traits>&
  operator>>(basic_istream<_CharT, _Traits>&, philox_engine<_UInt, _Wp, _Np, _Rp, _Cp...>&);
};
template <class _UIntType, size_t __w, size_t __n, size_t __r, _UIntType... __consts>
_LIBCPP_HIDE_FROM_ABI bool operator==(const philox_engine<_UIntType, __w, __n, __r, __consts...>& __x,
                                      const philox_engine<_UIntType, __w, __n, __r, __consts...>& __y) {
  return __x.__key_ == __y.__key_ && __x.__counter_ == __y.__counter_ && __x.__index_ == __y.__index_;
}
template <class _UIntType, size_t __w, size_t __n, size_t __r, _UIntType... __consts>
_LIBCPP_HIDE_FROM_ABI bool operator!=(const philox_engine<_UIntType, __w, __n, __r, __consts...>& __x,
                                      const philox_engine<_UIntType, __w, __n, __r, __consts...>& __y) {
  return !(__x == __y);
}
template <class _CharT, class _Traits, class _UIntType, size_t __w, size_t __n, size_t __r, _UIntType... __consts>
_LIBCPP_HIDE_FROM_ABI basic_ostream<_CharT, _Traits>&
operator<<(basic_ostream<_CharT, _Traits>& __os, const philox_engine<_UIntType, __w, __n, __r, __consts...>& __x) {
  __save_flags<_CharT, _Traits> __flags(__os);
  typedef basic_ostream<_CharT, _Traits> _Ostream;
  __os.flags(_Ostream::dec | _Ostream::left);
  const _CharT __space = __os.widen(' ');
  __os.fill(__space);
  for (size_t __j = 0; __j != __n / 2; ++__j)
    __os << __x.__key_[__j] << __space;
  for (size_t __j = 0; __j != __n; ++__j)
    __os << __x.__counter_[__j] << __space;
  return __os << __x.__index_;
}
template <class _CharT, class _Traits, class _UIntType, size_t __w, size_t __n, size_t __r, _UIntType... __consts>
_LIBCPP_HIDE_FROM_ABI basic_istream<_CharT, _Traits>&
operator>>(basic_istream<_CharT, _Traits>& __is, philox_engine<_UIntType, __w, __n, __r, __consts...>& __x) {
  __save_flags<_CharT, _Traits> __flags(__is);
  typedef basic_istream<_CharT, _Traits> _Istream;
  __is.flags(_Istream::dec | _Istream::skipws);
  array<_UIntType, __n / 2> __key;
  array<_UIntType, __n> __counter;
  size_t __index;
  for (size_t __j = 0; __j != __n / 2; ++__j)
    __is >> __key[__j];
  for (size_t __j = 0; __j != __n; ++__j)
    __is >> __counter[__j];
  __is >> __index;
  if (!__is.fail() && __index < __n) {
    __x.__key_     = __key;
    __x.__counter_ = __counter;
    __x.__index_   = __index;
    __x.__reconstruct_output();
  }
  return __is;
}
using philox4x32 = philox_engine<uint_fast32_t, 32, 4, 10, 0xCD9E8D57, 0x9E3779B9, 0xD2511F53, 0xBB67AE85>;
using philox4x64 =
    philox_engine<uint_fast64_t,
                  64,
                  4,
                  10,
                  0xCA5A826395121157ULL,
                  0x9E3779B97F4A7C15ULL,
                  0xD2E7470EE14C6C93ULL,
                  0xBB67AE8584CAA73BULL>;
_LIBCPP_END_NAMESPACE_STD
_LIBCPP_POP_MACROS
#endif
