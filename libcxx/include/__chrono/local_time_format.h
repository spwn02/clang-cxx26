// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___CHRONO_LOCAL_TIME_FORMAT_H
#define _LIBCPP___CHRONO_LOCAL_TIME_FORMAT_H

#include <__chrono/calendar.h>
#include <__chrono/duration.h>
#include <__chrono/time_point.h>
#include <__config>
#include <__fwd/string.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 20

namespace chrono {

// [time.format]: local-time-format-t (exposition only) and local_time_format. The formatter that uses the abbreviation
// and the offset for %Z and %z is in <__chrono/formatter.h>.
template <class _Duration>
struct __local_time_format_t {
  local_time<_Duration> __time_;
  const string* __abbrev_;
  const seconds* __offset_sec_;
};

template <class _Duration>
_LIBCPP_HIDE_FROM_ABI __local_time_format_t<_Duration>
local_time_format(local_time<_Duration> __time, const string* __abbrev = nullptr, const seconds* __offset_sec = nullptr) {
  return {__time, __abbrev, __offset_sec};
}

} // namespace chrono

#endif // _LIBCPP_STD_VER >= 20

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___CHRONO_LOCAL_TIME_FORMAT_H
