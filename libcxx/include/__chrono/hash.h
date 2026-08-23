// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___CHRONO_HASH_H
#define _LIBCPP___CHRONO_HASH_H

#include <__chrono/day.h>
#include <__chrono/duration.h>
#include <__chrono/month.h>
#include <__chrono/month_weekday.h>
#include <__chrono/monthday.h>
#include <__chrono/time_point.h>
#include <__chrono/weekday.h>
#include <__chrono/year.h>
#include <__chrono/year_month.h>
#include <__chrono/year_month_day.h>
#include <__chrono/year_month_weekday.h>
#include <__config>
#include <__functional/hash.h>
#include <cstddef>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

// [time.hash]
#if _LIBCPP_STD_VER >= 26

_LIBCPP_BEGIN_NAMESPACE_STD

template <class _Rep, class _Period>
struct hash<__enable_hash_helper<chrono::duration<_Rep, _Period>, _Rep> > {
  _LIBCPP_HIDE_FROM_ABI size_t operator()(const chrono::duration<_Rep, _Period>& __d) const {
    return hash<_Rep>()(__d.count());
  }
};

template <class _Clock, class _Duration>
struct hash<__enable_hash_helper<chrono::time_point<_Clock, _Duration>, _Duration> > {
  _LIBCPP_HIDE_FROM_ABI size_t operator()(const chrono::time_point<_Clock, _Duration>& __tp) const {
    return hash<_Duration>()(__tp.time_since_epoch());
  }
};

template <>
struct hash<chrono::day> {
  _LIBCPP_HIDE_FROM_ABI size_t operator()(const chrono::day& __d) const noexcept {
    return hash<unsigned>()(static_cast<unsigned>(__d));
  }
};

template <>
struct hash<chrono::month> {
  _LIBCPP_HIDE_FROM_ABI size_t operator()(const chrono::month& __m) const noexcept {
    return hash<unsigned>()(static_cast<unsigned>(__m));
  }
};

template <>
struct hash<chrono::year> {
  _LIBCPP_HIDE_FROM_ABI size_t operator()(const chrono::year& __y) const noexcept {
    return hash<int>()(static_cast<int>(__y));
  }
};

template <>
struct hash<chrono::weekday> {
  _LIBCPP_HIDE_FROM_ABI size_t operator()(const chrono::weekday& __wd) const noexcept {
    return hash<unsigned>()(__wd.c_encoding());
  }
};

template <>
struct hash<chrono::weekday_indexed> {
  _LIBCPP_HIDE_FROM_ABI size_t operator()(const chrono::weekday_indexed& __wdi) const noexcept {
    return std::__hash_combine(hash<chrono::weekday>()(__wdi.weekday()), hash<unsigned>()(__wdi.index()));
  }
};

template <>
struct hash<chrono::weekday_last> {
  _LIBCPP_HIDE_FROM_ABI size_t operator()(const chrono::weekday_last& __wdl) const noexcept {
    return hash<chrono::weekday>()(__wdl.weekday());
  }
};

template <>
struct hash<chrono::month_day> {
  _LIBCPP_HIDE_FROM_ABI size_t operator()(const chrono::month_day& __md) const noexcept {
    return std::__hash_combine(hash<chrono::month>()(__md.month()), hash<chrono::day>()(__md.day()));
  }
};

template <>
struct hash<chrono::month_day_last> {
  _LIBCPP_HIDE_FROM_ABI size_t operator()(const chrono::month_day_last& __mdl) const noexcept {
    return hash<chrono::month>()(__mdl.month());
  }
};

template <>
struct hash<chrono::month_weekday> {
  _LIBCPP_HIDE_FROM_ABI size_t operator()(const chrono::month_weekday& __mwd) const noexcept {
    return std::__hash_combine(
        hash<chrono::month>()(__mwd.month()), hash<chrono::weekday_indexed>()(__mwd.weekday_indexed()));
  }
};

template <>
struct hash<chrono::month_weekday_last> {
  _LIBCPP_HIDE_FROM_ABI size_t operator()(const chrono::month_weekday_last& __mwdl) const noexcept {
    return std::__hash_combine(
        hash<chrono::month>()(__mwdl.month()), hash<chrono::weekday_last>()(__mwdl.weekday_last()));
  }
};

template <>
struct hash<chrono::year_month> {
  _LIBCPP_HIDE_FROM_ABI size_t operator()(const chrono::year_month& __ym) const noexcept {
    return std::__hash_combine(hash<chrono::year>()(__ym.year()), hash<chrono::month>()(__ym.month()));
  }
};

template <>
struct hash<chrono::year_month_day> {
  _LIBCPP_HIDE_FROM_ABI size_t operator()(const chrono::year_month_day& __ymd) const noexcept {
    return std::__hash_combine(
        std::__hash_combine(hash<chrono::year>()(__ymd.year()), hash<chrono::month>()(__ymd.month())),
        hash<chrono::day>()(__ymd.day()));
  }
};

template <>
struct hash<chrono::year_month_day_last> {
  _LIBCPP_HIDE_FROM_ABI size_t operator()(const chrono::year_month_day_last& __ymdl) const noexcept {
    return std::__hash_combine(
        hash<chrono::year>()(__ymdl.year()), hash<chrono::month_day_last>()(__ymdl.month_day_last()));
  }
};

template <>
struct hash<chrono::year_month_weekday> {
  _LIBCPP_HIDE_FROM_ABI size_t operator()(const chrono::year_month_weekday& __ymwd) const noexcept {
    return std::__hash_combine(
        std::__hash_combine(hash<chrono::year>()(__ymwd.year()), hash<chrono::month>()(__ymwd.month())),
        hash<chrono::weekday_indexed>()(__ymwd.weekday_indexed()));
  }
};

template <>
struct hash<chrono::year_month_weekday_last> {
  _LIBCPP_HIDE_FROM_ABI size_t operator()(const chrono::year_month_weekday_last& __ymwdl) const noexcept {
    return std::__hash_combine(
        std::__hash_combine(hash<chrono::year>()(__ymwdl.year()), hash<chrono::month>()(__ymwdl.month())),
        hash<chrono::weekday_last>()(__ymwdl.weekday_last()));
  }
};

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_STD_VER >= 26

#endif // _LIBCPP___CHRONO_HASH_H
