//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef _LIBCPP___IO_FILE_FILE_BASE_H
#define _LIBCPP___IO_FILE_FILE_BASE_H

#include <__config>
#include <__io/support/config.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_IO_FILES

_LIBCPP_PUSH_MACROS
#  include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

// P4100R1 [8.8]; provisional specification: Corosio's file_base, the flags and the seek origins that stream_file and
// random_access_file share. The flags are combined with bitwise OR. The access mode is read_only, write_only or
// read_write; create, exclusive (with create: fail with errc::file_exists if the file exists), truncate and append
// (every write goes to the end of the file) are the usual creation and write modes; sync_all_on_write makes every
// write reach the storage before it completes.
struct file_base {
  enum flags : unsigned {
    read_only         = 1,
    write_only        = 2,
    read_write        = read_only | write_only,
    append            = 4,
    create            = 8,
    exclusive         = 16,
    truncate          = 32,
    sync_all_on_write = 64
  };

  // The origin of a seek: the start of the file, the current position, the end of the file.
  enum seek_basis { seek_set, seek_cur, seek_end };

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI friend constexpr flags operator|(flags __a, flags __b) noexcept {
    return static_cast<flags>(static_cast<unsigned>(__a) | static_cast<unsigned>(__b));
  }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI friend constexpr flags operator&(flags __a, flags __b) noexcept {
    return static_cast<flags>(static_cast<unsigned>(__a) & static_cast<unsigned>(__b));
  }
  _LIBCPP_HIDE_FROM_ABI friend constexpr flags& operator|=(flags& __a, flags __b) noexcept { return __a = __a | __b; }
  _LIBCPP_HIDE_FROM_ABI friend constexpr flags& operator&=(flags& __a, flags __b) noexcept { return __a = __a & __b; }

protected:
  file_base() noexcept                 = default;
  file_base(const file_base&) noexcept = default;
  ~file_base()                         = default;
};

// The type of the flags, for code that does not name the class.
using open_flags _LIBCPP_NODEBUG = file_base::flags;

} // namespace io

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP_HAS_IO_FILES

#endif // _LIBCPP___IO_FILE_FILE_BASE_H
