//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___IO_FILE_SUPPORT_WIN32_FILE_H
#define _LIBCPP___IO_FILE_SUPPORT_WIN32_FILE_H

#include <__config>
#include <__io/support/config.h>

#if _LIBCPP_HAS_IO_FILES && _LIBCPP_HAS_IO_WINSOCK
#  include <__cstddef/size_t.h>
#  include <__io/support/winsock.h>
#  include <__io_file/file_base.h>
#  include <__system_error/errc.h>
#  include <__system_error/error_category.h>
#  include <__system_error/error_code.h>
#  include <cstdint>
#  include <cstring>
#  include <string>
#endif

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_IO_FILES && _LIBCPP_HAS_IO_WINSOCK

_LIBCPP_PUSH_MACROS
#  include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

// The platform layer of <io_file> for Windows: the same functions with the same meaning as posix_file.h, which
// describes the contract. The handles are synchronous (the helper threads of the context block in them, like on POSIX
// systems); a positioned transfer passes the offset in an OVERLAPPED structure, which a synchronous handle honors, so
// that it does not depend on the file position. The handle is kept in an int, the way the sockets are (the value of a
// handle is small); -1 is INVALID_HANDLE_VALUE. There is no vectored transfer on a synchronous handle: the buffers are
// transferred one after the other, up to the first short transfer.

using __native_file _LIBCPP_NODEBUG           = int;
inline constexpr __native_file __invalid_file = -1;

inline constexpr size_t __file_max_buffers = 16;

struct __file_iovec {
  const void* __base_;
  size_t __len_;
};

_LIBCPP_HIDE_FROM_ABI inline void __file_set_iovec(__file_iovec& __v, const void* __p, size_t __n) noexcept {
  __v.__base_ = __p;
  __v.__len_  = __n;
}

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline HANDLE __to_handle(__native_file __fd) noexcept {
  return reinterpret_cast<HANDLE>(static_cast<intptr_t>(__fd));
}

// The Windows error numbers that the layer meets, as POSIX values.
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code __file_error(DWORD __e) noexcept {
  int __r;
  switch (__e) {
  case ERROR_FILE_NOT_FOUND:
  case ERROR_PATH_NOT_FOUND:
  case ERROR_INVALID_NAME:
    __r = ENOENT;
    break;
  case ERROR_ACCESS_DENIED:
  case ERROR_SHARING_VIOLATION:
  case ERROR_LOCK_VIOLATION:
    __r = EACCES;
    break;
  case ERROR_FILE_EXISTS:
  case ERROR_ALREADY_EXISTS:
    __r = EEXIST;
    break;
  case ERROR_INVALID_HANDLE:
    __r = EBADF;
    break;
  case ERROR_BROKEN_PIPE:
  case ERROR_NO_DATA:
    __r = EPIPE;
    break;
  case ERROR_DISK_FULL:
  case ERROR_HANDLE_DISK_FULL:
    __r = ENOSPC;
    break;
  case ERROR_NEGATIVE_SEEK:
  case ERROR_INVALID_PARAMETER:
    __r = EINVAL;
    break;
  case ERROR_SEEK_ON_DEVICE:
  case ERROR_INVALID_FUNCTION:
    __r = ESPIPE;
    break;
  case ERROR_FILENAME_EXCED_RANGE:
    __r = ENAMETOOLONG;
    break;
  case ERROR_NOT_ENOUGH_MEMORY:
  case ERROR_OUTOFMEMORY:
    __r = ENOMEM;
    break;
  case ERROR_DIRECTORY:
    __r = EISDIR;
    break;
  case ERROR_OPERATION_ABORTED:
    __r = ECANCELED;
    break;
  default:
    return error_code(static_cast<int>(__e), system_category());
  }
  return error_code(__r, generic_category());
}

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code __file_last_error() noexcept {
  return io::__file_error(::GetLastError());
}

// Opens the path (a NUL-terminated UTF-8 string) with the flags of file_base.
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code
__file_open(const char* __path, file_base::flags __f, __native_file& __fd) noexcept {
  DWORD __access = 0;
  switch (static_cast<unsigned>(__f) & 3u) {
  case static_cast<unsigned>(file_base::read_write):
    __access = GENERIC_READ | GENERIC_WRITE;
    break;
  case static_cast<unsigned>(file_base::write_only):
    __access = GENERIC_WRITE;
    break;
  default:
    __access = GENERIC_READ;
    break;
  }
  bool __create = (__f & file_base::create) != file_base::flags(0);
  bool __excl   = (__f & file_base::exclusive) != file_base::flags(0);
  bool __trunc  = (__f & file_base::truncate) != file_base::flags(0);
  bool __append = (__f & file_base::append) != file_base::flags(0);
  DWORD __disp  = OPEN_EXISTING;
  if (__create && __excl)
    __disp = CREATE_NEW;
  else if (__create && __trunc)
    __disp = CREATE_ALWAYS;
  else if (__create)
    __disp = OPEN_ALWAYS;
  else if (__trunc)
    __disp = TRUNCATE_EXISTING;
  if (__trunc)
    __access |= GENERIC_WRITE;                // TRUNCATE_EXISTING and CREATE_ALWAYS need write access
  if (__append && (__access & GENERIC_WRITE)) // every write goes to the end: append access without write access
    __access = (__access & ~DWORD(GENERIC_WRITE)) | FILE_APPEND_DATA | FILE_WRITE_ATTRIBUTES | FILE_GENERIC_WRITE;
  if (__append && (__access & FILE_GENERIC_WRITE) == FILE_GENERIC_WRITE)
    __access &= ~DWORD(FILE_WRITE_DATA);
  DWORD __attrs = FILE_ATTRIBUTE_NORMAL;
  if ((__f & file_base::sync_all_on_write) != file_base::flags(0))
    __attrs |= FILE_FLAG_WRITE_THROUGH;
  int __wlen = ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, __path, -1, nullptr, 0);
  if (__wlen <= 0)
    return make_error_code(errc::invalid_argument);
  wstring __w(static_cast<size_t>(__wlen), L'\0');
  ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, __path, -1, &__w[0], __wlen);
  HANDLE __h = ::CreateFileW(
      __w.c_str(), __access, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, __disp, __attrs, nullptr);
  if (__h == INVALID_HANDLE_VALUE)
    return io::__file_last_error();
  __fd = static_cast<__native_file>(reinterpret_cast<intptr_t>(__h));
  return error_code();
}

_LIBCPP_HIDE_FROM_ABI inline void __file_close(__native_file __fd) noexcept {
  if (__fd != __invalid_file)
    (void)::CloseHandle(io::__to_handle(__fd));
}

// One transfer of at most 1 GiB (the length of ReadFile and WriteFile is a DWORD). __off null: at the position of the
// handle.
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code __file_xfer1(
    bool __read, __native_file __fd, const void* __buf, size_t __len, const uint64_t* __off, size_t& __got) noexcept {
  DWORD __chunk = __len > 0x40000000u ? 0x40000000u : static_cast<DWORD>(__len);
  DWORD __n     = 0;
  OVERLAPPED __ov;
  OVERLAPPED* __po = nullptr;
  if (__off) {
    ::memset(&__ov, 0, sizeof(__ov));
    __ov.Offset     = static_cast<DWORD>(*__off & 0xffffffffu);
    __ov.OffsetHigh = static_cast<DWORD>(*__off >> 32);
    __po            = &__ov;
  }
  BOOL __ok = __read ? ::ReadFile(io::__to_handle(__fd), const_cast<void*>(__buf), __chunk, &__n, __po)
                     : ::WriteFile(io::__to_handle(__fd), __buf, __chunk, &__n, __po);
  if (!__ok) {
    DWORD __e = ::GetLastError();
    if (__read && (__e == ERROR_HANDLE_EOF || __e == ERROR_BROKEN_PIPE)) {
      __got = 0;
      return error_code();
    }
    if (__e == ERROR_ACCESS_DENIED) // a transfer the access mode of the handle does not allow
      return error_code(EBADF, generic_category());
    return io::__file_error(__e);
  }
  __got = __n;
  return error_code();
}

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code __file_xfer(
    bool __read,
    __native_file __fd,
    const __file_iovec* __v,
    size_t __n,
    const uint64_t* __off,
    size_t& __got) noexcept {
  size_t __total = 0;
  for (size_t __i = 0; __i != __n; ++__i) {
    if (__v[__i].__len_ == 0)
      continue;
    uint64_t __o = __off ? *__off + __total : 0;
    size_t __r   = 0;
    if (error_code __ec =
            io::__file_xfer1(__read, __fd, __v[__i].__base_, __v[__i].__len_, __off ? &__o : nullptr, __r)) {
      if (__total != 0)
        break;
      return __ec;
    }
    __total += __r;
    if (__r != __v[__i].__len_)
      break;
  }
  __got = __total;
  return error_code();
}

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code
__file_read(__native_file __fd, const __file_iovec* __v, size_t __n, size_t& __got) noexcept {
  return io::__file_xfer(true, __fd, __v, __n, nullptr, __got);
}

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code
__file_write(__native_file __fd, const __file_iovec* __v, size_t __n, size_t& __got) noexcept {
  return io::__file_xfer(false, __fd, __v, __n, nullptr, __got);
}

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline bool __file_offset_ok(uint64_t __off) noexcept {
  return __off <= static_cast<uint64_t>(INT64_MAX);
}

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code
__file_pread(__native_file __fd, const __file_iovec* __v, size_t __n, uint64_t __off, size_t& __got) noexcept {
  if (!io::__file_offset_ok(__off))
    return make_error_code(errc::invalid_argument);
  return io::__file_xfer(true, __fd, __v, __n, &__off, __got);
}

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code
__file_pwrite(__native_file __fd, const __file_iovec* __v, size_t __n, uint64_t __off, size_t& __got) noexcept {
  if (!io::__file_offset_ok(__off))
    return make_error_code(errc::invalid_argument);
  return io::__file_xfer(false, __fd, __v, __n, &__off, __got);
}

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code __file_size(__native_file __fd, uint64_t& __size) noexcept {
  LARGE_INTEGER __li;
  if (!::GetFileSizeEx(io::__to_handle(__fd), &__li))
    return io::__file_last_error();
  __size = static_cast<uint64_t>(__li.QuadPart);
  return error_code();
}

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code __file_resize(__native_file __fd, uint64_t __size) noexcept {
  if (!io::__file_offset_ok(__size))
    return make_error_code(errc::invalid_argument);
  FILE_END_OF_FILE_INFO __info;
  __info.EndOfFile.QuadPart = static_cast<LONGLONG>(__size);
  if (!::SetFileInformationByHandle(io::__to_handle(__fd), FileEndOfFileInfo, &__info, sizeof(__info)))
    return io::__file_last_error();
  return error_code();
}

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code __file_sync_data(__native_file __fd) noexcept {
  return ::FlushFileBuffers(io::__to_handle(__fd)) ? error_code() : io::__file_last_error();
}

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code __file_sync_all(__native_file __fd) noexcept {
  return ::FlushFileBuffers(io::__to_handle(__fd)) ? error_code() : io::__file_last_error();
}

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline error_code
__file_seek(__native_file __fd, int64_t __off, file_base::seek_basis __whence, uint64_t& __pos) noexcept {
  DWORD __w = FILE_BEGIN;
  if (__whence == file_base::seek_cur)
    __w = FILE_CURRENT;
  else if (__whence == file_base::seek_end)
    __w = FILE_END;
  LARGE_INTEGER __d, __np;
  __d.QuadPart = __off;
  if (!::SetFilePointerEx(io::__to_handle(__fd), __d, &__np, __w))
    return io::__file_last_error();
  __pos = static_cast<uint64_t>(__np.QuadPart);
  return error_code();
}

} // namespace io

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP_HAS_IO_FILES && _LIBCPP_HAS_IO_WINSOCK

#endif // _LIBCPP___IO_FILE_SUPPORT_WIN32_FILE_H
