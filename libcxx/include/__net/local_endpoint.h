//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef _LIBCPP___NET_LOCAL_ENDPOINT_H
#define _LIBCPP___NET_LOCAL_ENDPOINT_H

#include <__config>
#include <__io/support/config.h>
#include <__io_stream/error.h>
#include <__system_error/errc.h>
#include <__system_error/error_code.h>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_IO_SOCKETS

_LIBCPP_PUSH_MACROS
#  include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

// P4100R1 [8.9]; provisional specification: Corosio's local_endpoint. The address of a Unix domain socket: a path in a
// fixed-size buffer (no allocation, trivially copyable). A path whose first character is '\0' names an abstract
// socket (Linux only; the other systems report an error when it is bound or
// connected): the leading NUL is part of the stored path. The path is never unlinked by the library.
class local_endpoint {
  char __path_[108] = {};
  uint8_t __len_    = 0;

public:
  // The longest path that fits the address of the platform (sizeof sun_path - 1: 107 on Linux, 103 on macOS and BSD).
#  if defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) ||                     \
      defined(__DragonFly__)
  static constexpr size_t max_path_length = 103;
#  else
  static constexpr size_t max_path_length = 107;
#  endif

  _LIBCPP_HIDE_FROM_ABI constexpr local_endpoint() noexcept = default;

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI string_view path() const noexcept { return string_view(__path_, __len_); }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr bool is_abstract() const noexcept {
    return __len_ > 0 && __path_[0] == '\0';
  }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr bool empty() const noexcept { return __len_ == 0; }

  // The path, or "[abstract:name]" for an abstract socket.
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI string to_string() const {
    if (is_abstract())
      return "[abstract:" + string(__path_ + 1, __len_ - 1) + "]";
    return string(__path_, __len_);
  }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI friend bool
  operator==(const local_endpoint& __a, const local_endpoint& __b) noexcept {
    return __a.path() == __b.path();
  }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI friend strong_ordering
  operator<=>(const local_endpoint& __a, const local_endpoint& __b) noexcept {
    return __a.path() <=> __b.path();
  }

private:
  // A path longer than max_path_length cannot be represented (errc::filename_too_long); an empty path is the unbound
  // endpoint.
  _LIBCPP_HIDE_FROM_ABI friend io_result<local_endpoint> make_local_endpoint(string_view __p) noexcept;
};

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline io_result<local_endpoint> make_local_endpoint(string_view __p) noexcept {
  if (__p.size() > local_endpoint::max_path_length)
    return io_result<local_endpoint>(make_error_code(errc::filename_too_long), local_endpoint());
  local_endpoint __e;
  for (size_t __i = 0; __i != __p.size(); ++__i)
    __e.__path_[__i] = __p[__i];
  __e.__len_ = static_cast<uint8_t>(__p.size());
  return io_result<local_endpoint>(error_code(), __e);
}

} // namespace io

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP_HAS_IO_SOCKETS

#endif // _LIBCPP___NET_LOCAL_ENDPOINT_H
