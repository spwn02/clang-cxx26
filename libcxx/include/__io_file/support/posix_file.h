//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef _LIBCPP___IO_FILE_SUPPORT_POSIX_FILE_H
#define _LIBCPP___IO_FILE_SUPPORT_POSIX_FILE_H

#include <__config>
#include <__io/support/config.h>

#if _LIBCPP_HAS_IO_FILES
#  include <__cstddef/size_t.h>
#  include <__io_file/file_base.h>
#  include <__system_error/errc.h>
#  include <__system_error/error_category.h>
#  include <__system_error/error_code.h>
// The system headers use names such as __in and __out for their parameters, which are macros of some platform SDKs (SAL
// annotations) and of the tests of <__config> hygiene; they are hidden while the headers are included.
#  pragma push_macro("__in")
#  undef __in
#  pragma push_macro("__out")
#  undef __out
#  pragma push_macro("__inout")
#  undef __inout
#  pragma push_macro("__pre")
#  undef __pre
#  pragma push_macro("__post")
#  undef __post
#  pragma push_macro("__deref")
#  undef __deref
#  pragma push_macro("__bound")
#  undef __bound
#  pragma push_macro("__nz")
#  undef __nz
#  pragma push_macro("__part")
#  undef __part
#  pragma push_macro("__full")
#  undef __full
#  pragma push_macro("__allocator")
#  undef __allocator
#  pragma push_macro("__deallocate")
#  undef __deallocate
#  pragma push_macro("__format_string")
#  undef __format_string
#  pragma push_macro("__input")
#  undef __input
#  pragma push_macro("__output")
#  undef __output
#  pragma push_macro("__acquire")
#  undef __acquire
#  pragma push_macro("__release")
#  undef __release
#  include <cerrno>
#  include <cstddef>
#  include <cstdint>
#  include <fcntl.h>
#  include <sys/stat.h>
#  include <sys/types.h>
#  include <sys/uio.h>
#  include <unistd.h>
#  pragma pop_macro("__release")
#  pragma pop_macro("__acquire")
#  pragma pop_macro("__output")
#  pragma pop_macro("__input")
#  pragma pop_macro("__format_string")
#  pragma pop_macro("__deallocate")
#  pragma pop_macro("__allocator")
#  pragma pop_macro("__full")
#  pragma pop_macro("__part")
#  pragma pop_macro("__nz")
#  pragma pop_macro("__bound")
#  pragma pop_macro("__deref")
#  pragma pop_macro("__post")
#  pragma pop_macro("__pre")
#  pragma pop_macro("__inout")
#  pragma pop_macro("__out")
#  pragma pop_macro("__in")
#endif

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_IO_FILES

_LIBCPP_PUSH_MACROS
#  include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

// The platform layer of <io_file> for POSIX file I/O (Linux): the only place that includes the system file headers. A
// Windows backend (overlapped I/O on the completion port) provides the same functions (the same names, the same
// meaning) in a sibling file.
//
// The calls block; the file operations of <io_file> call __file_read, __file_write, __file_pread and __file_pwrite from
// the helper threads of the context. The others are quick and are called directly. Every function is noexcept and
// reports failure as an error_code in generic_category (the errno value).

using __native_file _LIBCPP_NODEBUG           = int;
inline constexpr __native_file __invalid_file = -1;

// The most buffers of a sequence that one call transfers.
inline constexpr size_t __file_max_buffers = 16;

using __file_iovec _LIBCPP_NODEBUG = ::iovec;

_LIBCPP_HIDE_FROM_ABI inline void __file_set_iovec(__file_iovec& __v, const void* __p, size_t __n) noexcept {
  __v.iov_base = const_cast<void*>(__p);
  __v.iov_len  = __n;
}

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code __file_errno(int __e) noexcept {
  return error_code(__e, generic_category());
}

// Opens the path (a NUL-terminated string) with the flags of file_base.
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code
__file_open(const char* __path, file_base::flags __f, __native_file& __fd) noexcept {
  int __o = 0;
  switch (static_cast<unsigned>(__f) & 3u) {
  case static_cast<unsigned>(file_base::read_write):
    __o = O_RDWR;
    break;
  case static_cast<unsigned>(file_base::write_only):
    __o = O_WRONLY;
    break;
  default:
    __o = O_RDONLY;
    break;
  }
  if ((__f & file_base::create) != file_base::flags(0))
    __o |= O_CREAT;
  if ((__f & file_base::exclusive) != file_base::flags(0))
    __o |= O_EXCL;
  if ((__f & file_base::truncate) != file_base::flags(0))
    __o |= O_TRUNC;
  if ((__f & file_base::append) != file_base::flags(0))
    __o |= O_APPEND;
  if ((__f & file_base::sync_all_on_write) != file_base::flags(0))
    __o |= O_SYNC;
  __o |= O_CLOEXEC;
  int __r;
  do {
    __r = ::open(__path, __o, 0666);
  } while (__r < 0 && errno == EINTR);
  if (__r < 0)
    return io::__file_errno(errno);
  __fd = __r;
  return error_code();
}

_LIBCPP_HIDE_FROM_ABI inline void __file_close(__native_file __fd) noexcept {
  if (__fd >= 0)
    ::close(__fd); // EINTR on close leaves the descriptor closed on Linux; there is no retry.
}

// Transfers at the position of the descriptor (a stream file).
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code
__file_read(__native_file __fd, const __file_iovec* __v, size_t __n, size_t& __got) noexcept {
  ssize_t __r;
  do {
    __r = ::readv(__fd, __v, static_cast<int>(__n));
  } while (__r < 0 && errno == EINTR);
  if (__r < 0)
    return io::__file_errno(errno);
  __got = static_cast<size_t>(__r);
  return error_code();
}

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code
__file_write(__native_file __fd, const __file_iovec* __v, size_t __n, size_t& __got) noexcept {
  ssize_t __r;
  do {
    __r = ::writev(__fd, __v, static_cast<int>(__n));
  } while (__r < 0 && errno == EINTR);
  if (__r < 0)
    return io::__file_errno(errno);
  __got = static_cast<size_t>(__r);
  return error_code();
}

// Transfers at an explicit offset (a random access file); thread-safe.
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline bool __file_offset_ok(uint64_t __off) noexcept {
  return __off <= static_cast<uint64_t>(INT64_MAX);
}

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code
__file_pread(__native_file __fd, const __file_iovec* __v, size_t __n, uint64_t __off, size_t& __got) noexcept {
  if (!io::__file_offset_ok(__off))
    return make_error_code(errc::invalid_argument);
  ssize_t __r;
  do {
    __r = ::preadv(__fd, __v, static_cast<int>(__n), static_cast<off_t>(__off));
  } while (__r < 0 && errno == EINTR);
  if (__r < 0)
    return io::__file_errno(errno);
  __got = static_cast<size_t>(__r);
  return error_code();
}

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code
__file_pwrite(__native_file __fd, const __file_iovec* __v, size_t __n, uint64_t __off, size_t& __got) noexcept {
  if (!io::__file_offset_ok(__off))
    return make_error_code(errc::invalid_argument);
  ssize_t __r;
  do {
    __r = ::pwritev(__fd, __v, static_cast<int>(__n), static_cast<off_t>(__off));
  } while (__r < 0 && errno == EINTR);
  if (__r < 0)
    return io::__file_errno(errno);
  __got = static_cast<size_t>(__r);
  return error_code();
}

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code __file_size(__native_file __fd, uint64_t& __size) noexcept {
  struct stat __st;
  if (::fstat(__fd, &__st) < 0)
    return io::__file_errno(errno);
  __size = static_cast<uint64_t>(__st.st_size);
  return error_code();
}

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code __file_resize(__native_file __fd, uint64_t __size) noexcept {
  if (!io::__file_offset_ok(__size))
    return make_error_code(errc::invalid_argument);
  int __r;
  do {
    __r = ::ftruncate(__fd, static_cast<off_t>(__size));
  } while (__r < 0 && errno == EINTR);
  return __r < 0 ? io::__file_errno(errno) : error_code();
}

// Flushes the data (and the metadata needed to read it back) / the data and all the metadata.
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code __file_sync_data(__native_file __fd) noexcept {
  int __r;
  do {
    __r = ::fdatasync(__fd);
  } while (__r < 0 && errno == EINTR);
  return __r < 0 ? io::__file_errno(errno) : error_code();
}

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code __file_sync_all(__native_file __fd) noexcept {
  int __r;
  do {
    __r = ::fsync(__fd);
  } while (__r < 0 && errno == EINTR);
  return __r < 0 ? io::__file_errno(errno) : error_code();
}

// Moves the position of the descriptor; __pos is the new position from the start of the file.
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code
__file_seek(__native_file __fd, int64_t __off, file_base::seek_basis __whence, uint64_t& __pos) noexcept {
  int __w = SEEK_SET;
  if (__whence == file_base::seek_cur)
    __w = SEEK_CUR;
  else if (__whence == file_base::seek_end)
    __w = SEEK_END;
  off_t __r = ::lseek(__fd, static_cast<off_t>(__off), __w);
  if (__r < 0)
    return io::__file_errno(errno);
  __pos = static_cast<uint64_t>(__r);
  return error_code();
}

} // namespace io

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP_HAS_IO_FILES

#endif // _LIBCPP___IO_FILE_SUPPORT_POSIX_FILE_H
