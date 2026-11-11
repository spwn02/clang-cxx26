//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef _LIBCPP___IO_FILE_FILE_H
#define _LIBCPP___IO_FILE_FILE_H

#include <__config>
#include <__coroutine/coroutine_handle.h>
#include <__coroutine/coroutine_traits.h>
#include <__cstddef/size_t.h>
#include <__io/execution_context.h>
#include <__io/executor.h>
#include <__io/io_env.h>
#include <__io/support/config.h>
#include <__io/task.h>
#include <__io_buffer/buffer.h>
#include <__io_buffer/buffer_sequence.h>
#include <__io_stream/error.h>
#include <__memory/addressof.h>
#include <__memory/shared_ptr.h>
#include <__system_error/errc.h>
#include <__system_error/error_code.h>
#include <__type_traits/is_convertible.h>
#include <__type_traits/is_same.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/move.h>
#include <cstdint>
#include <string>
#include <string_view>

#if _LIBCPP_HAS_IO_FILES
#  include <__io_context/blocking_service.h>
#  include <__io_file/file_base.h>
#  include <__io_file/support/posix_file.h>
#endif

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_IO_FILES

_LIBCPP_PUSH_MACROS
#  include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

// The open descriptor of a file. Shared between the file object and the operations in progress, so that close() (and
// the destruction) of the file cannot invalidate the descriptor under a call that runs on a helper thread: the
// descriptor is closed when the last of them lets go of it.
struct __file_handle {
  const __native_file __fd_;
  bool __owned_ = true; // false once release() has handed the descriptor out

  _LIBCPP_HIDE_FROM_ABI explicit __file_handle(__native_file __fd) noexcept : __fd_(__fd) {}
  __file_handle(const __file_handle&)            = delete;
  __file_handle& operator=(const __file_handle&) = delete;
  _LIBCPP_HIDE_FROM_ABI ~__file_handle() {
    if (__owned_)
      io::__file_close(__fd_);
  }
};

enum class __file_kind : unsigned char { __read, __write, __pread, __pwrite };

// One transfer of a file: readv, writev, preadv or pwritev, on a helper thread. It lives in the frame of the coroutine
// that awaits it, together with the descriptors of the buffers.
class __file_awaiter final : public __blocking_op {
  shared_ptr<__file_handle> __h_;
  __file_kind __kind_;
  uint64_t __off_;
  size_t __cnt_ = 0;
  size_t __got_ = 0;
  __file_iovec __iov_[__file_max_buffers];

  _LIBCPP_HIDE_FROM_ABI static void __run(__blocking_op* __b) noexcept {
    auto* __self     = static_cast<__file_awaiter*>(__b);
    const int __fd   = __self->__h_->__fd_;
    const auto* __v  = __self->__iov_;
    const size_t __n = __self->__cnt_;
    switch (__self->__kind_) {
    case __file_kind::__read:
      __self->__ec_ = io::__file_read(__fd, __v, __n, __self->__got_);
      break;
    case __file_kind::__write:
      __self->__ec_ = io::__file_write(__fd, __v, __n, __self->__got_);
      break;
    case __file_kind::__pread:
      __self->__ec_ = io::__file_pread(__fd, __v, __n, __self->__off_, __self->__got_);
      break;
    case __file_kind::__pwrite:
      __self->__ec_ = io::__file_pwrite(__fd, __v, __n, __self->__off_, __self->__got_);
      break;
    }
  }

public:
  _LIBCPP_HIDE_FROM_ABI
  __file_awaiter(__blocking_service* __s, shared_ptr<__file_handle> __h, __file_kind __k, uint64_t __off) noexcept
      : __blocking_op(__s, __h.get()), __h_(std::move(__h)), __kind_(__k), __off_(__off) {
    __work_ = &__run;
  }

  // Describes up to __file_max_buffers non-empty buffers of the sequence; returns their number.
  template <class _Buf, class _Seq>
  _LIBCPP_HIDE_FROM_ABI size_t __fill(const _Seq& __seq) noexcept {
    size_t __n = 0;
    for (auto __it = io::buffer_sequence_begin(__seq), __last = io::buffer_sequence_end(__seq);
         __it != __last && __n != __file_max_buffers;
         ++__it) {
      _Buf __b = static_cast<_Buf>(*__it);
      if (__b.size() != 0)
        io::__file_set_iovec(__iov_[__n++], __b.data(), __b.size());
    }
    return __cnt_ = __n;
  }

  _LIBCPP_HIDE_FROM_ABI io_result<size_t> await_resume() noexcept {
    __finish();
    if (__ec_)
      return io_result<size_t>(__ec_, 0);
    return io_result<size_t>(error_code(), __got_);
  }
};

// The transfer as a task: a transfer of nothing completes at once, a read of 0 bytes (with a non-empty buffer) is the
// end of the file. A file that is not open completes with errc::bad_file_descriptor.
template <class _Buf, class _Seq>
_LIBCPP_HIDE_FROM_ABI task<io_result<size_t>>
__file_transfer(__blocking_service* __s, shared_ptr<__file_handle> __h, __file_kind __k, uint64_t __off, _Seq __seq) {
  if (!__h)
    co_return io_result<size_t>(make_error_code(errc::bad_file_descriptor), 0);
  __file_awaiter __aw(__s, std::move(__h), __k, __off);
  if (__aw.template __fill<_Buf>(__seq) == 0)
    co_return io_result<size_t>(error_code(), 0);
  auto [__ec, __n] = co_await __aw;
  if (__ec)
    co_return io_result<size_t>(__ec, 0);
  if (__n == 0 && (__k == __file_kind::__read || __k == __file_kind::__pread))
    co_return io_result<size_t>(make_error_code(error::eof), 0);
  co_return io_result<size_t>(error_code(), __n);
}

// What stream_file and random_access_file have in common: the descriptor and its management (P4100R1 [8.8];
// provisional specification: Corosio). A file belongs to an io_context (the constructors take the context or one of its
// executors); its transfers run as blocking calls on helper threads of the context (see __blocking_service) and
// complete on the executor of the awaiting coroutine.
//
// Cancellation: cancel(), close() and a stop request of the awaiting coroutine complete an operation that waits for a
// helper thread at once with error::canceled. An operation whose call is in progress cannot be interrupted; it
// completes with the result of the call (deviation: Corosio completes it with operation_canceled once the call has
// returned; the bytes of a completed transfer are not hidden here). close() does not wait for the calls in progress:
// the descriptor is closed when the last of them has returned.
//
// Distinct files can be used concurrently. Operations on one file may run concurrently (random_access_file: any number;
// stream_file: they are serialized by the kernel at the file position); open(), close(), resize(), release() and
// assign() need external synchronization. The context must outlive the file, and the file must outlive its operations.
// Errors are the errno value of the system call, in generic_category().
class __file_common : public file_base {
  execution_context* __ctx_;
  __blocking_service* __svc_;
  shared_ptr<__file_handle> __h_;

protected:
  _LIBCPP_HIDE_FROM_ABI explicit __file_common(execution_context& __c)
      : __ctx_(std::addressof(__c)), __svc_(std::addressof(__c.use_service<__blocking_service>())) {}

  // Precondition: no operation is pending on __o.
  _LIBCPP_HIDE_FROM_ABI __file_common(__file_common&& __o) noexcept
      : __ctx_(__o.__ctx_), __svc_(__o.__svc_), __h_(std::move(__o.__h_)) {}
  // Closes *this; precondition: no operation is pending on __o.
  _LIBCPP_HIDE_FROM_ABI __file_common& operator=(__file_common&& __o) noexcept {
    if (this != std::addressof(__o)) {
      close();
      __ctx_ = __o.__ctx_;
      __svc_ = __o.__svc_;
      __h_   = std::move(__o.__h_);
    }
    return *this;
  }
  _LIBCPP_HIDE_FROM_ABI ~__file_common() { close(); }

  _LIBCPP_HIDE_FROM_ABI __blocking_service* __service() const noexcept { return __svc_; }
  _LIBCPP_HIDE_FROM_ABI const shared_ptr<__file_handle>& __handle() const noexcept { return __h_; }

public:
  __file_common(const __file_common&)            = delete;
  __file_common& operator=(const __file_common&) = delete;

  // Opens the file (closing the open one first). A failure (a missing file, a permission, errc::file_exists with
  // exclusive) is returned, not thrown. A path with an embedded NUL is errc::invalid_argument. Deviation: the path is a
  // string_view or a type with a native() member that converts to one (std::filesystem::path), so that <io_file> does
  // not include <filesystem>; Corosio takes std::filesystem::path.
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI error_code open(string_view __path, file_base::flags __f = read_only) noexcept {
    close();
    if (__path.find('\0') != string_view::npos)
      return make_error_code(errc::invalid_argument);
    __native_file __fd = __invalid_file;
#  if _LIBCPP_HAS_EXCEPTIONS
    try {
#  endif
      string __p(__path);
      if (error_code __ec = io::__file_open(__p.c_str(), __f, __fd))
        return __ec;
      __h_ = std::make_shared<__file_handle>(__fd);
      return error_code();
#  if _LIBCPP_HAS_EXCEPTIONS
    } catch (...) {
      io::__file_close(__fd);
      return make_error_code(errc::not_enough_memory);
    }
#  endif
  }
  template <class _Path>
    requires(!is_convertible_v<const _Path&, string_view> && requires(const _Path& __p) { string_view(__p.native()); })
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI error_code open(const _Path& __path, file_base::flags __f = read_only) noexcept {
    return open(string_view(__path.native()), __f);
  }
  // A path whose native() is not a narrow string (std::filesystem::path on Windows holds wide characters): its UTF-8 form.
  template <class _Path>
    requires(!is_convertible_v<const _Path&, string_view> && !requires(const _Path& __p) { string_view(__p.native()); } &&
             requires(const _Path& __p) { __p.u8string().data(); })
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI error_code open(const _Path& __path, file_base::flags __f = read_only) noexcept {
    auto __u = __path.u8string();
    string __s(reinterpret_cast<const char*>(__u.data()), __u.size());
    return open(string_view(__s), __f);
  }

  // Cancels the operations that wait for a thread and gives up the file (see the class comment). Idempotent.
  _LIBCPP_HIDE_FROM_ABI void close() noexcept {
    if (!__h_)
      return;
    cancel();
    __h_.reset();
  }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI bool is_open() const noexcept { return __h_ != nullptr; }

  // Each operation that waits for a thread completes with error::canceled; the operations that are started later are
  // not affected.
  _LIBCPP_HIDE_FROM_ABI void cancel() noexcept {
    if (__h_)
      __svc_->__cancel_owner(__h_.get());
  }

  // The descriptor, or -1 if the file is not open.
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI int native_handle() const noexcept { return __h_ ? __h_->__fd_ : __invalid_file; }

  // The size of the file in bytes. These five are quick calls made on the calling thread (fsync can take long; use a
  // helper of your own if that matters). A closed file is errc::bad_file_descriptor.
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI io_result<uint64_t> size() const noexcept {
    uint64_t __s = 0;
    if (!__h_)
      return io_result<uint64_t>(make_error_code(errc::bad_file_descriptor), 0);
    if (error_code __ec = io::__file_size(__h_->__fd_, __s))
      return io_result<uint64_t>(__ec, 0);
    return io_result<uint64_t>(error_code(), __s);
  }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI error_code resize(uint64_t __new_size) noexcept {
    return __h_ ? io::__file_resize(__h_->__fd_, __new_size) : make_error_code(errc::bad_file_descriptor);
  }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI error_code sync_data() noexcept {
    return __h_ ? io::__file_sync_data(__h_->__fd_) : make_error_code(errc::bad_file_descriptor);
  }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI error_code sync_all() noexcept {
    return __h_ ? io::__file_sync_all(__h_->__fd_) : make_error_code(errc::bad_file_descriptor);
  }

  // Cancels the waiting operations and gives the descriptor to the caller, who must close it; the file is closed.
  // Precondition: no operation is in progress (one that is would go on using the descriptor).
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI io_result<int> release() noexcept {
    if (!__h_)
      return io_result<int>(make_error_code(errc::bad_file_descriptor), __invalid_file);
    cancel();
    int __fd       = __h_->__fd_;
    __h_->__owned_ = false;
    __h_.reset();
    return io_result<int>(error_code(), __fd);
  }
  // Takes ownership of an open descriptor (closing the open file first); on failure the descriptor stays with the
  // caller.
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI error_code assign(int __fd) noexcept {
    if (__fd < 0)
      return make_error_code(errc::bad_file_descriptor);
    shared_ptr<__file_handle> __n;
#  if _LIBCPP_HAS_EXCEPTIONS
    try {
#  endif
      __n = std::make_shared<__file_handle>(__fd);
#  if _LIBCPP_HAS_EXCEPTIONS
    } catch (...) {
      return make_error_code(errc::not_enough_memory);
    }
#  endif
    close();
    __h_ = std::move(__n);
    return error_code();
  }
};

// P4100R1 [8.8]; provisional specification: Corosio's random_access_file. Reads and writes at explicit byte offsets
// (pread and pwrite, so there is no position and any number of operations can run at the same time). It does not
// satisfy ReadStream or WriteStream. A read at or beyond the end of the file completes with error::eof and 0 bytes; a
// write beyond the end extends the file (the gap reads as zeros). With append, Linux appends every write whatever the
// offset.
class random_access_file : public __file_common {
public:
  _LIBCPP_HIDE_FROM_ABI explicit random_access_file(execution_context& __c) : __file_common(__c) {}
  template <Executor _Ex>
    requires(!is_same_v<remove_cvref_t<_Ex>, random_access_file>)
  _LIBCPP_HIDE_FROM_ABI explicit random_access_file(const _Ex& __ex) : __file_common(__ex.context()) {}

  _LIBCPP_HIDE_FROM_ABI random_access_file(random_access_file&&) noexcept            = default;
  _LIBCPP_HIDE_FROM_ABI random_access_file& operator=(random_access_file&&) noexcept = default;

  // Reads into the first buffers of the sequence (up to 16 non-empty ones) at the offset; completes with the bytes
  // read. The sequence is copied into the operation; the bytes it refers to must stay valid until it completes.
  template <MutableBufferSequence _Seq>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI task<io_result<size_t>> read_some_at(uint64_t __offset, _Seq __seq) {
    return io::__file_transfer<mutable_buffer, _Seq>(
        __service(), __handle(), __file_kind::__pread, __offset, std::move(__seq));
  }
  template <ConstBufferSequence _Seq>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI task<io_result<size_t>> write_some_at(uint64_t __offset, _Seq __seq) {
    return io::__file_transfer<const_buffer, _Seq>(
        __service(), __handle(), __file_kind::__pwrite, __offset, std::move(__seq));
  }
};

// P4100R1 [8.8]; provisional specification: Corosio's stream_file. A file read and written sequentially from a position
// that the operations advance (the position of the descriptor, which seek() moves); it satisfies ReadStream,
// WriteStream and Stream, so it converts to any_stream. A read at the end of the file completes with error::eof and 0
// bytes. Deviation: Corosio keeps the position in the object and uses pread/pwrite; this one uses the position of the
// descriptor (read, write, lseek), which also works for the descriptors that are not seekable (assign()).
class stream_file : public __file_common {
public:
  _LIBCPP_HIDE_FROM_ABI explicit stream_file(execution_context& __c) : __file_common(__c) {}
  template <Executor _Ex>
    requires(!is_same_v<remove_cvref_t<_Ex>, stream_file>)
  _LIBCPP_HIDE_FROM_ABI explicit stream_file(const _Ex& __ex) : __file_common(__ex.context()) {}

  _LIBCPP_HIDE_FROM_ABI stream_file(stream_file&&) noexcept            = default;
  _LIBCPP_HIDE_FROM_ABI stream_file& operator=(stream_file&&) noexcept = default;

  // P4100R1 [8.6]: complete with the bytes transferred; read_some reports error::eof (and 0) at the end of the file.
  // The sequence is copied into the operation; the bytes it refers to must stay valid until it completes.
  template <MutableBufferSequence _Seq>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI task<io_result<size_t>> read_some(_Seq __seq) {
    return io::__file_transfer<mutable_buffer, _Seq>(__service(), __handle(), __file_kind::__read, 0, std::move(__seq));
  }
  template <ConstBufferSequence _Seq>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI task<io_result<size_t>> write_some(_Seq __seq) {
    return io::__file_transfer<const_buffer, _Seq>(__service(), __handle(), __file_kind::__write, 0, std::move(__seq));
  }

  // Moves the position; completes at once (on the calling thread) with the new position from the start of the file. A
  // position before the start is errc::invalid_argument; a file that cannot seek (a pipe) is errc::invalid_seek.
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI io_result<uint64_t>
  seek(int64_t __offset, file_base::seek_basis __origin = seek_set) noexcept {
    uint64_t __pos = 0;
    if (!__handle())
      return io_result<uint64_t>(make_error_code(errc::bad_file_descriptor), 0);
    if (error_code __ec = io::__file_seek(__handle()->__fd_, __offset, __origin, __pos))
      return io_result<uint64_t>(__ec, 0);
    return io_result<uint64_t>(error_code(), __pos);
  }
};

} // namespace io

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP_HAS_IO_FILES

#endif // _LIBCPP___IO_FILE_FILE_H
