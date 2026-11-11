//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef _LIBCPP___NET_RESOLVER_ERROR_H
#define _LIBCPP___NET_RESOLVER_ERROR_H

#include <__config>
#include <__io/support/config.h>
#include <__system_error/errc.h>
#include <__system_error/error_category.h>
#include <__system_error/error_code.h>
#include <__system_error/error_condition.h>
#include <__type_traits/integral_constant.h>
#include <string>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_IO_SOCKETS

_LIBCPP_PUSH_MACROS
#  include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

// P4100R1 [8.9]; provisional specification: Corosio's resolver. The failures of a name lookup (getaddrinfo and
// getnameinfo report them in their own number space, EAI_*). Deviation: Corosio reports them as generic errno values
// (EAI_NONAME as no_such_device_or_address); here they have a category of their own that is documented, and each
// code is equivalent to the generic condition Corosio reports, so that
// `ec == errc::no_such_device_or_address` is still true for host_not_found.
enum class resolver_error {
  host_not_found = 1,     // EAI_NONAME: the host or the service is not known (equivalent to no_such_device_or_address)
  no_data,                // EAI_NODATA, EAI_ADDRFAMILY: the host exists but has no address of the requested kind
  try_again,              // EAI_AGAIN: a temporary failure of the name service (resource_unavailable_try_again)
  no_recovery,            // EAI_FAIL: a permanent failure of the name service (io_error)
  service_not_found,      // EAI_SERVICE: the service is not known for the socket type (invalid_argument)
  family_not_supported,   // EAI_FAMILY (address_family_not_supported)
  socktype_not_supported, // EAI_SOCKTYPE (not_supported)
  bad_flags,              // EAI_BADFLAGS: an invalid combination of flags (invalid_argument)
  out_of_memory,          // EAI_MEMORY (not_enough_memory)
  overflow,               // EAI_OVERFLOW: a result did not fit (value_too_large)
  unknown                 // any other value
};

_LIBCPP_HIDE_FROM_ABI inline const error_category& resolver_category() noexcept;

_LIBCPP_DIAGNOSTIC_PUSH
_LIBCPP_CLANG_DIAGNOSTIC_IGNORED("-Wweak-vtables")
class __resolver_category_t final : public error_category {
public:
  _LIBCPP_HIDE_FROM_ABI __resolver_category_t() noexcept = default;
  _LIBCPP_HIDE_FROM_ABI_VIRTUAL const char* name() const noexcept override { return "std::io resolver"; }
  _LIBCPP_HIDE_FROM_ABI_VIRTUAL string message(int __ev) const override {
    switch (static_cast<resolver_error>(__ev)) {
    case resolver_error::host_not_found:
      return "host or service not found";
    case resolver_error::no_data:
      return "the host has no address of the requested kind";
    case resolver_error::try_again:
      return "temporary failure in name resolution";
    case resolver_error::no_recovery:
      return "non-recoverable failure in name resolution";
    case resolver_error::service_not_found:
      return "service not supported for the socket type";
    case resolver_error::family_not_supported:
      return "address family not supported";
    case resolver_error::socktype_not_supported:
      return "socket type not supported";
    case resolver_error::bad_flags:
      return "invalid resolver flags";
    case resolver_error::out_of_memory:
      return "out of memory in name resolution";
    case resolver_error::overflow:
      return "resolver result too large";
    case resolver_error::unknown:
      break;
    }
    return "unknown resolver error";
  }
  _LIBCPP_HIDE_FROM_ABI_VIRTUAL error_condition default_error_condition(int __ev) const noexcept override {
    switch (static_cast<resolver_error>(__ev)) {
    case resolver_error::host_not_found:
    case resolver_error::no_data:
      return make_error_condition(errc::no_such_device_or_address);
    case resolver_error::try_again:
      return make_error_condition(errc::resource_unavailable_try_again);
    case resolver_error::no_recovery:
      return make_error_condition(errc::io_error);
    case resolver_error::service_not_found:
    case resolver_error::bad_flags:
      return make_error_condition(errc::invalid_argument);
    case resolver_error::family_not_supported:
      return make_error_condition(errc::address_family_not_supported);
    case resolver_error::socktype_not_supported:
      return make_error_condition(errc::not_supported);
    case resolver_error::out_of_memory:
      return make_error_condition(errc::not_enough_memory);
    case resolver_error::overflow:
      return make_error_condition(errc::value_too_large);
    case resolver_error::unknown:
      break;
    }
    return error_condition(__ev, resolver_category());
  }
};
_LIBCPP_DIAGNOSTIC_POP

_LIBCPP_HIDE_FROM_ABI inline const error_category& resolver_category() noexcept {
  static const __resolver_category_t __c;
  return __c;
}

_LIBCPP_HIDE_FROM_ABI inline error_code make_error_code(resolver_error __e) noexcept {
  return error_code(static_cast<int>(__e), resolver_category());
}

} // namespace io

template <>
struct is_error_code_enum<io::resolver_error> : true_type {};

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP_HAS_IO_SOCKETS

#endif // _LIBCPP___NET_RESOLVER_ERROR_H
