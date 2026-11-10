//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef _LIBCPP___NET_SOCKET_CORE_H
#define _LIBCPP___NET_SOCKET_CORE_H

#include <__config>
#include <__coroutine/coroutine_handle.h>
#include <__coroutine/coroutine_traits.h>
#include <__io/execution_context.h>
#include <__io/io_env.h>
#include <__io/support/config.h>
#include <__io/task.h>
#include <__io_buffer/buffer.h>
#include <__io_buffer/buffer_sequence.h>
#include <__io_context/op.h>
#include <__io_context/reactor.h>
#include <__io_context/reactor_wait.h>
#include <__io_stream/error.h>
#include <__memory/addressof.h>
#include <__mutex/lock_guard.h>
#include <__mutex/mutex.h>
#include <__net/family.h>
#include <__net/support/posix_socket.h>
#include <__system_error/errc.h>
#include <__system_error/error_code.h>
#include <__utility/exchange.h>
#include <__utility/move.h>
#include <__utility/swap.h>
#include <cstddef>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_IO_SOCKETS

_LIBCPP_PUSH_MACROS
#  include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

// Up to __sock_max_buffers non-empty buffers of the sequence.
template <class _Buf, class _Seq>
_LIBCPP_HIDE_FROM_ABI size_t __collect_buffers(const _Seq& __seq, _Buf* __dst) noexcept {
  size_t __n = 0;
  for (auto __it = io::buffer_sequence_begin(__seq), __last = io::buffer_sequence_end(__seq);
       __it != __last && __n != __sock_max_buffers;
       ++__it) {
    _Buf __b = static_cast<_Buf>(*__it);
    if (__b.size() != 0)
      __dst[__n++] = __b;
  }
  return __n;
}

class __socket_core;

// A link of the list of the waits that are pending on one socket, so that cancel() and close() can reach them.
struct __wait_node {
  __wait_node* __prev_ = nullptr;
  __wait_node* __next_ = nullptr;
  __op_base* __op_     = nullptr;
};

// A wait for readiness of the descriptor of a socket (a __reactor_wait that is registered at the socket). It lives in
// the frame of the operation that awaits it.
class __sock_wait_awaiter : public __reactor_wait_awaiter {
  __socket_core* __core_;
  __wait_node __node_;

public:
  _LIBCPP_HIDE_FROM_ABI __sock_wait_awaiter(__socket_core& __c, unsigned __events) noexcept;
  _LIBCPP_HIDE_FROM_ABI __sock_wait_awaiter(__sock_wait_awaiter&& __o) noexcept
      : __reactor_wait_awaiter(std::move(__o)), __core_(__o.__core_) {} // NOLINT(bugprone-use-after-move)

  _LIBCPP_HIDE_FROM_ABI bool await_suspend(coroutine_handle<> __h, const io_env* __env);
  _LIBCPP_HIDE_FROM_ABI io_result<unsigned> await_resume() noexcept;
};

// The state shared by every socket and acceptor: the context, the descriptor and the registry of the pending waits.
// Not copyable; the move constructor and move assignment have the precondition that no operation is pending.
//
// Every operation is a coroutine that tries the system call, and waits for the descriptor to become ready (through the
// reactor of the context) when the call would block. The wait is registered here: cancel() cancels all of them (they
// complete with error::canceled, and the operation reports it), and close() cancels first. Completions are posted to
// the executor of the awaiting coroutine, never run inside cancel().
class __socket_core {
  friend class __sock_wait_awaiter;

  execution_context* __ctx_;
  __native_socket __fd_ = __invalid_socket;
  io::family __family_  = io::family::v4;
  mutex __m_;
  __wait_node* __head_ = nullptr;

  _LIBCPP_HIDE_FROM_ABI void __register(__wait_node* __n) noexcept {
    lock_guard<mutex> __lk(__m_);
    __n->__prev_ = nullptr;
    __n->__next_ = __head_;
    if (__head_)
      __head_->__prev_ = __n;
    __head_ = __n;
  }
  _LIBCPP_HIDE_FROM_ABI void __unregister(__wait_node* __n) noexcept {
    lock_guard<mutex> __lk(__m_);
    if (__n->__prev_)
      __n->__prev_->__next_ = __n->__next_;
    else if (__head_ == __n)
      __head_ = __n->__next_;
    else
      return; // not registered
    if (__n->__next_)
      __n->__next_->__prev_ = __n->__prev_;
    __n->__prev_ = __n->__next_ = nullptr;
  }

protected:
  _LIBCPP_HIDE_FROM_ABI explicit __socket_core(execution_context& __c) noexcept : __ctx_(std::addressof(__c)) {}
  // Takes the descriptor and the context of __o. Precondition: no operation is pending on __o.
  _LIBCPP_HIDE_FROM_ABI __socket_core(__socket_core&& __o) noexcept
      : __ctx_(__o.__ctx_), __fd_(std::exchange(__o.__fd_, __invalid_socket)), __family_(__o.__family_) {}
  _LIBCPP_HIDE_FROM_ABI __socket_core& operator=(__socket_core&& __o) noexcept {
    if (this != std::addressof(__o)) {
      __close();
      __ctx_    = __o.__ctx_;
      __fd_     = std::exchange(__o.__fd_, __invalid_socket);
      __family_ = __o.__family_;
    }
    return *this;
  }
  _LIBCPP_HIDE_FROM_ABI ~__socket_core() { __close(); }

  _LIBCPP_HIDE_FROM_ABI execution_context& __context() const noexcept { return *__ctx_; }
  _LIBCPP_HIDE_FROM_ABI io::family __family() const noexcept { return __family_; }

  _LIBCPP_HIDE_FROM_ABI error_code __open(__sock_domain __d, __sock_type __t = __sock_type::__stream) noexcept {
    if (__fd_ != __invalid_socket)
      return error_code();
    __native_socket __s;
    if (error_code __ec = io::__sock_open(__d, __s, __t))
      return __ec;
    __fd_     = __s;
    __family_ = __d == __sock_domain::__v6 ? io::family::v6 : io::family::v4;
    return error_code();
  }
  // Adopts an accepted descriptor.
  _LIBCPP_HIDE_FROM_ABI void __adopt(__native_socket __s, io::family __f) noexcept {
    __close();
    __fd_     = __s;
    __family_ = __f;
  }

  _LIBCPP_HIDE_FROM_ABI void __close() noexcept {
    if (__fd_ == __invalid_socket)
      return;
    cancel();
    io::__sock_close(std::exchange(__fd_, __invalid_socket));
  }

  // Cancels the pending operations and gives up the descriptor without closing it.
  _LIBCPP_HIDE_FROM_ABI __native_socket __detach() noexcept {
    cancel();
    return std::exchange(__fd_, __invalid_socket);
  }

  _LIBCPP_HIDE_FROM_ABI __sock_wait_awaiter __wait_for(unsigned __events) noexcept {
    return __sock_wait_awaiter(*this, __events);
  }

  // The stream operations: read into the first buffers of the sequence (error::eof at the end of the stream), write the
  // first buffers; both complete with the bytes transferred.
  template <class _Seq>
  _LIBCPP_HIDE_FROM_ABI task<io_result<size_t>> __read_some(_Seq __seq) {
    mutable_buffer __bufs[__sock_max_buffers];
    size_t __n = io::__collect_buffers(__seq, __bufs);
    if (__n == 0)
      co_return io_result<size_t>(error_code(), 0);
    for (;;) {
      size_t __got    = 0;
      error_code __ec = io::__sock_receive(__fd_, __bufs, __n, __got);
      if (!__ec) {
        if (__got == 0)
          co_return io_result<size_t>(make_error_code(error::eof), 0);
        co_return io_result<size_t>(error_code(), __got);
      }
      if (!io::__is_would_block(__ec))
        co_return io_result<size_t>(__ec, 0);
      auto [__wec, __ev] = co_await __wait_for(__poll_read);
      if (__wec)
        co_return io_result<size_t>(__wec, 0);
    }
  }

  template <class _Seq>
  _LIBCPP_HIDE_FROM_ABI task<io_result<size_t>> __write_some(_Seq __seq) {
    const_buffer __bufs[__sock_max_buffers];
    size_t __n = io::__collect_buffers(__seq, __bufs);
    if (__n == 0)
      co_return io_result<size_t>(error_code(), 0);
    for (;;) {
      size_t __got    = 0;
      error_code __ec = io::__sock_send(__fd_, __bufs, __n, __got);
      if (!__ec)
        co_return io_result<size_t>(error_code(), __got);
      if (!io::__is_would_block(__ec))
        co_return io_result<size_t>(__ec, 0);
      auto [__wec, __ev] = co_await __wait_for(__poll_write);
      if (__wec)
        co_return io_result<size_t>(__wec, 0);
    }
  }

  // Accepts one connection: the new descriptor and its address.
  _LIBCPP_HIDE_FROM_ABI task<io_result<__native_socket>> __accept_raw() {
    for (;;) {
      __native_socket __s;
      __sockaddr_buf __peer;
      error_code __ec = io::__sock_accept(__fd_, __s, __peer);
      if (!__ec)
        co_return io_result<__native_socket>(error_code(), __s);
      if (__ec == errc::connection_aborted || __ec == errc::interrupted)
        continue;
      if (!io::__is_would_block(__ec))
        co_return io_result<__native_socket>(__ec, __invalid_socket);
      auto [__wec, __ev] = co_await __wait_for(__poll_read);
      if (__wec)
        co_return io_result<__native_socket>(__wec, __invalid_socket);
    }
  }

  // Connects to the address, opening the socket for the domain first if necessary.
  _LIBCPP_HIDE_FROM_ABI task<io_result<>>
  __connect_raw(__sock_domain __d, __sockaddr_buf __addr, __sock_type __t = __sock_type::__stream) {
    if (error_code __ec = __open(__d, __t))
      co_return io_result<>(__ec);
    error_code __ec = io::__sock_connect(__fd_, __addr);
    if (!__ec)
      co_return io_result<>(error_code());
    if (!io::__is_in_progress(__ec))
      co_return io_result<>(__ec);
    auto [__wec, __ev] = co_await __wait_for(__poll_write);
    if (__wec)
      co_return io_result<>(__wec);
    co_return io_result<>(io::__sock_pending_error(__fd_));
  }

public:
  __socket_core(const __socket_core&)            = delete;
  __socket_core& operator=(const __socket_core&) = delete;

  // Cancels every pending operation of the socket: each completes with error::canceled (unless its result was already
  // decided). The operations that are started later are not affected.
  _LIBCPP_HIDE_FROM_ABI void cancel() noexcept {
    lock_guard<mutex> __lk(__m_);
    for (__wait_node* __n = __head_; __n;) {
      __wait_node* __next = __n->__next_;
      __n->__op_->__cancel_(__n->__op_);
      __n = __next;
    }
  }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI bool is_open() const noexcept { return __fd_ != __invalid_socket; }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI __native_socket native_handle() const noexcept { return __fd_; }

  // Applies a socket option. Fails with errc::bad_file_descriptor if the socket is not open, or with the error of the
  // system.
  template <class _Option>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI error_code set_option(const _Option& __opt) noexcept {
    if (!is_open())
      return make_error_code(errc::bad_file_descriptor);
    return __opt.__set(__fd_);
  }
  // The current value of an option; the option is value-initialized on failure.
  template <class _Option>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI io_result<_Option> get_option() const noexcept {
    if (!is_open())
      return io_result<_Option>(make_error_code(errc::bad_file_descriptor), _Option{});
    _Option __opt{};
    if (error_code __ec = __opt.__get(__fd_))
      return io_result<_Option>(__ec, _Option{});
    return io_result<_Option>(error_code(), __opt);
  }
};

_LIBCPP_HIDE_FROM_ABI inline __sock_wait_awaiter::__sock_wait_awaiter(__socket_core& __c, unsigned __events) noexcept
    : __reactor_wait_awaiter(__c.__context(), __c.__fd_, __events), __core_(std::addressof(__c)) {}

_LIBCPP_HIDE_FROM_ABI inline bool __sock_wait_awaiter::await_suspend(coroutine_handle<> __h, const io_env* __env) {
  __node_.__op_ = this;
  __core_->__register(std::addressof(__node_));
  return __reactor_wait_awaiter::await_suspend(__h, __env);
}

_LIBCPP_HIDE_FROM_ABI inline io_result<unsigned> __sock_wait_awaiter::await_resume() noexcept {
  io_result<unsigned> __r = __reactor_wait_awaiter::await_resume();
  __core_->__unregister(std::addressof(__node_));
  return __r;
}

// What both kinds of stream socket have in common: reading, writing, shutdown.
class __stream_socket_base : public __socket_core {
protected:
  using __socket_core::__socket_core;

public:
  // P4100R1 [8.6]: complete with the bytes transferred; read_some reports error::eof (and 0) at the end of the stream.
  // The buffer sequence is copied into the operation; the bytes it refers to must stay valid until it completes.
  template <MutableBufferSequence _Seq>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI task<io_result<size_t>> read_some(_Seq __seq) {
    return this->template __read_some<_Seq>(std::move(__seq));
  }
  template <ConstBufferSequence _Seq>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI task<io_result<size_t>> write_some(_Seq __seq) {
    return this->template __write_some<_Seq>(std::move(__seq));
  }

  // Disables sending and/or receiving. Shutting down the sending side makes the peer's reads report error::eof.
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI error_code shutdown(shutdown_type __what) noexcept {
    if (!is_open())
      return make_error_code(errc::bad_file_descriptor);
    return io::__sock_shutdown(native_handle(), __what);
  }
};

} // namespace io

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP_HAS_IO_SOCKETS

#endif // _LIBCPP___NET_SOCKET_CORE_H
