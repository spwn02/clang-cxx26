//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___IO_STREAM_ERROR_H
#define _LIBCPP___IO_STREAM_ERROR_H

#include <__config>
#include <__system_error/errc.h>
#include <__system_error/error_category.h>
#include <__system_error/error_code.h>
#include <__system_error/error_condition.h>
#include <__tuple/tuple_size.h>
#include <string>
#include <tuple>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_EXPERIMENTAL_NETWORKING && _LIBCPP_HAS_THREADS

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

// The error codes produced by the stream operations and algorithms (Capy: capy::error). Code values are
// implementation details; compare a received error_code against the portable io::cond conditions.
enum class error { eof = 1, canceled, stream_truncated, timeout };

// The portable conditions the codes above are equivalent to (Capy: capy::cond).
enum class cond { eof = 1, canceled, stream_truncated, timeout };

// P4100R1 [8.6]: an operation result is the error_code followed by the operation's values, so that
// `auto [ec, n] = co_await s.read_some(b);` works. Capy calls this `results`; the name here follows the task text.
// Whether the values are meaningful when ec is set is defined by the operation (read and write report the byte count
// transferred before the condition arose).
template <class... _Ts>
using io_result = tuple<error_code, _Ts...>;

_LIBCPP_HIDE_FROM_ABI inline const error_category& io_category() noexcept;

_LIBCPP_DIAGNOSTIC_PUSH
_LIBCPP_CLANG_DIAGNOSTIC_IGNORED("-Wweak-vtables")
class __cond_category_t final : public error_category {
public:
  _LIBCPP_HIDE_FROM_ABI __cond_category_t() noexcept = default;
  _LIBCPP_HIDE_FROM_ABI_VIRTUAL const char* name() const noexcept override { return "std::io condition"; }
  _LIBCPP_HIDE_FROM_ABI_VIRTUAL string message(int __ev) const override {
    switch (static_cast<cond>(__ev)) {
    case cond::eof:
      return "end of stream";
    case cond::canceled:
      return "operation canceled";
    case cond::stream_truncated:
      return "stream truncated";
    case cond::timeout:
      return "operation timed out";
    }
    return "unknown std::io condition";
  }
  // Like Capy, the conditions also match the portable system conditions that mean the same thing, so that a stream
  // built on a platform API (which reports operation_canceled or timed_out) compares equal to
  // io::cond::canceled/timeout.
  _LIBCPP_HIDE_FROM_ABI_VIRTUAL bool equivalent(const error_code& __ec, int __cond) const noexcept override {
    if (__ec.category() == io_category_ref())
      return __ec.value() == __cond;
    switch (static_cast<cond>(__cond)) {
    case cond::canceled:
      return __ec == make_error_condition(errc::operation_canceled);
    case cond::timeout:
      return __ec == make_error_condition(errc::timed_out);
    default:
      return false;
    }
  }

private:
  _LIBCPP_HIDE_FROM_ABI static const error_category& io_category_ref() noexcept;
};
_LIBCPP_DIAGNOSTIC_POP

_LIBCPP_HIDE_FROM_ABI inline const error_category& io_condition_category() noexcept {
  static const __cond_category_t __c;
  return __c;
}

_LIBCPP_HIDE_FROM_ABI inline const error_category& __cond_category_t::io_category_ref() noexcept {
  return io_category();
}

_LIBCPP_DIAGNOSTIC_PUSH
_LIBCPP_CLANG_DIAGNOSTIC_IGNORED("-Wweak-vtables")
class __error_category_t final : public error_category {
public:
  _LIBCPP_HIDE_FROM_ABI __error_category_t() noexcept = default;
  _LIBCPP_HIDE_FROM_ABI_VIRTUAL const char* name() const noexcept override { return "std::io"; }
  _LIBCPP_HIDE_FROM_ABI_VIRTUAL string message(int __ev) const override {
    switch (static_cast<error>(__ev)) {
    case error::eof:
      return "end of stream";
    case error::canceled:
      return "operation canceled";
    case error::stream_truncated:
      return "peer closed the connection without a proper shutdown";
    case error::timeout:
      return "operation timed out";
    }
    return "unknown std::io error";
  }
  _LIBCPP_HIDE_FROM_ABI_VIRTUAL error_condition default_error_condition(int __ev) const noexcept override {
    return error_condition(__ev, io_condition_category());
  }
};
_LIBCPP_DIAGNOSTIC_POP

_LIBCPP_HIDE_FROM_ABI inline const error_category& io_category() noexcept {
  static const __error_category_t __c;
  return __c;
}

_LIBCPP_HIDE_FROM_ABI inline error_code make_error_code(error __e) noexcept {
  return error_code(static_cast<int>(__e), io_category());
}

_LIBCPP_HIDE_FROM_ABI inline error_condition make_error_condition(cond __c) noexcept {
  return error_condition(static_cast<int>(__c), io_condition_category());
}

} // namespace io

template <>
struct is_error_code_enum<io::error> : true_type {};

template <>
struct is_error_condition_enum<io::cond> : true_type {};

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_HAS_EXPERIMENTAL_NETWORKING && _LIBCPP_HAS_THREADS

#endif // _LIBCPP___IO_STREAM_ERROR_H
