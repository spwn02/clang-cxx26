//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef _LIBCPP___NET_TLS_TLS_STREAM_H
#define _LIBCPP___NET_TLS_TLS_STREAM_H

#include <__assert>
#include <__config>
#include <__coroutine/coroutine_handle.h>
#include <__coroutine/coroutine_traits.h>
#include <__io/support/config.h>
#include <__io/task.h>
#include <__io_buffer/buffer.h>
#include <__io_buffer/buffer_sequence.h>
#include <__io_stream/any_stream.h>
#include <__io_stream/error.h>
#include <__io_stream/stream_concepts.h>
#include <__memory/shared_ptr.h>
#include <__memory/unique_ptr.h>
#include <__mutex/lock_guard.h>
#include <__mutex/mutex.h>
#include <__net_tls/engine.h>
#include <__net_tls/error.h>
#include <__net_tls/openssl_engine.h>
#include <__net_tls/tls_context.h>
#include <__system_error/errc.h>
#include <__system_error/error_code.h>
#include <__type_traits/is_same.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/move.h>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_IO_TLS

_LIBCPP_PUSH_MACROS
#  include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

// P4100R1 [8.10]; provisional specification: Corosio's tls_stream/openssl_stream. TLS over any Stream. The stream wraps
// (does not own) a Stream, drives the cryptographic engine with the ciphertext it reads from and writes to the wrapped
// stream, and satisfies Stream itself, so it converts to any_stream. All operations are IoAwaitables (tasks): they are
// cancelled through the stop token of the awaiting coroutine, which reaches the wait on the wrapped stream, and report
// failures as error codes (tls_error of tls_category(); the codes of the wrapped stream unchanged; io::error::eof after
// the close_notify of the peer; io::error::stream_truncated if the wrapped stream ends without it).
//
// Thread safety: distinct objects are independent. One read and one write may be in flight at the same time (the
// engine calls are serialized internally, never across a suspension); shutdown() may overlap a pending read (it then
// sends the close_notify and returns without waiting for the one of the peer, which the pending read reports as
// io::error::eof). A cancelled operation can leave the TLS stream unusable: cancel the handshake and discard the
// stream. The stream and the wrapped stream must outlive their operations.
class tls_stream {
  struct __state {
    any_stream __own_; // the wrapper made by the pointer constructor
    any_stream* __next_ = nullptr;
    shared_ptr<__tls_ctx_data> __ctx_;
    unique_ptr<__tls_engine> __eng_;
    string __host_;
    string __detail_;
    mutex __mutex_;
    bool __flushing_    = false;
    bool __reading_     = false;
    bool __established_ = false;
    error_code __write_ec_;
    vector<char> __ibuf_ = vector<char>(__tls_record_size + 512);
    vector<char> __obuf_ = vector<char>(__tls_record_size + 512);

    _LIBCPP_HIDE_FROM_ABI error_code __engine_error() {
      lock_guard<mutex> __lk(__mutex_);
      __detail_ = __eng_->__error_detail();
      return __eng_->__error();
    }

    // Writes the ciphertext the engine has produced to the wrapped stream. Only one operation writes at a time; the
    // others leave their ciphertext in the engine for it. A failed write is sticky.
    _LIBCPP_HIDE_FROM_ABI task<error_code> __flush() {
      for (;;) {
        size_t __n;
        {
          lock_guard<mutex> __lk(__mutex_);
          if (__write_ec_)
            co_return __write_ec_;
          if (__flushing_)
            co_return error_code();
          __n = __eng_->__take_output(__obuf_.data(), __obuf_.size());
          if (__n == 0)
            co_return error_code();
          __flushing_ = true;
        }
        size_t __done = 0;
        error_code __ec;
        while (__done != __n) {
          auto [__wec, __w] = co_await __next_->write_some(const_buffer(__obuf_.data() + __done, __n - __done));
          __done += __w;
          if (__wec) {
            __ec = __wec;
            break;
          }
          if (__w == 0) {
            __ec = make_error_code(errc::io_error);
            break;
          }
        }
        {
          lock_guard<mutex> __lk(__mutex_);
          __flushing_ = false;
          if (__ec)
            __write_ec_ = __ec;
        }
        if (__ec)
          co_return __ec;
      }
    }

    // Reads ciphertext from the wrapped stream into the engine. The caller owns the read side (__reading_).
    _LIBCPP_HIDE_FROM_ABI task<error_code> __fill() {
      auto [__rec, __n] = co_await __next_->read_some(mutable_buffer(__ibuf_.data(), __ibuf_.size()));
      {
        lock_guard<mutex> __lk(__mutex_);
        if (__n != 0)
          __eng_->__feed(__ibuf_.data(), __n);
        __reading_ = false;
      }
      if (__rec == error::eof)
        co_return make_error_code(error::stream_truncated);
      co_return __rec;
    }

    _LIBCPP_HIDE_FROM_ABI task<io_result<>> __handshake(tls_role __role) {
      if (!__ctx_)
        co_return io_result<>(make_error_code(tls_error::bad_configuration));
      {
        lock_guard<mutex> __lk(__mutex_);
        __eng_.reset(new __openssl_engine(__ctx_));
        __established_ = false;
        __flushing_    = false;
        __reading_     = false;
        __write_ec_.clear();
        __detail_.clear();
        error_code __ec = __eng_->__start(__role, __role == tls_role::client ? string_view(__host_) : string_view());
        if (__ec) {
          __detail_ = __eng_->__error_detail();
          co_return io_result<>(__ec);
        }
      }
      for (;;) {
        __tls_io __st;
        {
          lock_guard<mutex> __lk(__mutex_);
          __st = __eng_->__handshake();
          if (__st == __tls_io::__want_read) {
            if (__reading_)
              co_return io_result<>(make_error_code(errc::device_or_resource_busy));
            __reading_ = true;
          }
        }
        error_code __fec = co_await __flush(); // also the alert of a failed handshake
        if (__st == __tls_io::__ok) {
          if (__fec)
            co_return io_result<>(__fec);
          lock_guard<mutex> __lk(__mutex_);
          __established_ = true;
          co_return io_result<>(error_code());
        }
        if (__st == __tls_io::__error)
          co_return io_result<>(__engine_error());
        if (__fec) {
          lock_guard<mutex> __lk(__mutex_);
          __reading_ = false;
          co_return io_result<>(__fec);
        }
        error_code __rec = co_await __fill();
        if (__rec)
          co_return io_result<>(__rec);
      }
    }

    template <class _Seq>
    _LIBCPP_HIDE_FROM_ABI task<io_result<size_t>> __read_some(_Seq __seq) {
      mutable_buffer __b;
      for (auto __it = io::buffer_sequence_begin(__seq), __last = io::buffer_sequence_end(__seq); __it != __last;
           ++__it) {
        mutable_buffer __c = static_cast<mutable_buffer>(*__it);
        if (__c.size() != 0) {
          __b = __c;
          break;
        }
      }
      if (__b.size() == 0)
        co_return io_result<size_t>(error_code(), 0);
      {
        lock_guard<mutex> __lk(__mutex_);
        if (!__established_)
          co_return io_result<size_t>(make_error_code(tls_error::not_established), 0);
      }
      for (;;) {
        size_t __got = 0;
        __tls_io __st;
        {
          lock_guard<mutex> __lk(__mutex_);
          __st = __eng_->__read(__b.data(), __b.size(), __got);
          if (__st == __tls_io::__want_read) {
            if (__reading_)
              co_return io_result<size_t>(make_error_code(errc::device_or_resource_busy), 0);
            __reading_ = true;
          }
        }
        error_code __fec = co_await __flush(); // key updates, alerts
        if (__st == __tls_io::__ok)
          co_return io_result<size_t>(error_code(), __got);
        if (__st == __tls_io::__closed)
          co_return io_result<size_t>(make_error_code(error::eof), 0);
        if (__st == __tls_io::__error)
          co_return io_result<size_t>(__engine_error(), 0);
        if (__fec) {
          lock_guard<mutex> __lk(__mutex_);
          __reading_ = false;
          co_return io_result<size_t>(__fec, 0);
        }
        error_code __rec = co_await __fill();
        if (__rec)
          co_return io_result<size_t>(__rec, 0);
      }
    }

    template <class _Seq>
    _LIBCPP_HIDE_FROM_ABI task<io_result<size_t>> __write_some(_Seq __seq) {
      const_buffer __b;
      for (auto __it = io::buffer_sequence_begin(__seq), __last = io::buffer_sequence_end(__seq); __it != __last;
           ++__it) {
        const_buffer __c = static_cast<const_buffer>(*__it);
        if (__c.size() != 0) {
          __b = __c;
          break;
        }
      }
      if (__b.size() == 0)
        co_return io_result<size_t>(error_code(), 0);
      size_t __put = 0;
      __tls_io __st;
      {
        lock_guard<mutex> __lk(__mutex_);
        if (!__established_)
          co_return io_result<size_t>(make_error_code(tls_error::not_established), 0);
        if (__write_ec_)
          co_return io_result<size_t>(__write_ec_, 0);
        __st = __eng_->__write(__b.data(), __b.size(), __put);
      }
      error_code __fec = co_await __flush();
      if (__st == __tls_io::__error)
        co_return io_result<size_t>(__engine_error(), 0);
      if (__fec)
        co_return io_result<size_t>(__fec, 0);
      co_return io_result<size_t>(error_code(), __put);
    }

    _LIBCPP_HIDE_FROM_ABI task<io_result<>> __shutdown() {
      {
        lock_guard<mutex> __lk(__mutex_);
        if (!__established_)
          co_return io_result<>(make_error_code(tls_error::not_established));
      }
      for (;;) {
        __tls_io __st;
        bool __wait = false;
        {
          lock_guard<mutex> __lk(__mutex_);
          __st = __eng_->__shutdown();
          if (__st == __tls_io::__want_read && !__reading_) {
            __reading_ = true;
            __wait     = true;
          }
        }
        error_code __fec = co_await __flush();
        if (__st == __tls_io::__error)
          co_return io_result<>(__engine_error());
        if (__fec) {
          if (__wait) {
            lock_guard<mutex> __lk(__mutex_);
            __reading_ = false;
          }
          co_return io_result<>(__fec);
        }
        if (__st != __tls_io::__want_read || !__wait)
          co_return io_result<>(error_code()); // done, or a read is pending and reports the close_notify of the peer
        error_code __rec = co_await __fill();
        if (__rec)
          co_return io_result<>(__rec);
      }
    }
  };

  unique_ptr<__state> __s_;

  _LIBCPP_HIDE_FROM_ABI static unique_ptr<__state> __make(const tls_context& __ctx) {
    auto __p    = make_unique<__state>();
    __p->__ctx_ = __ctx.__data();
    return __p;
  }

public:
  // Wraps the stream (which must outlive this object) with the configuration of the context. The context is read at the
  // first handshake; it must not be modified after this.
  template <Stream _Str>
    requires(!is_same_v<remove_cvref_t<_Str>, tls_stream> && !is_same_v<remove_cvref_t<_Str>, any_stream> &&
             requires(_Str& __o, span<const mutable_buffer> __r, span<const const_buffer> __w) {
               __o.read_some(__r);
               __o.write_some(__w);
             })
  _LIBCPP_HIDE_FROM_ABI tls_stream(_Str* __stream, const tls_context& __ctx) : __s_(__make(__ctx)) {
    __s_->__own_  = any_stream(__stream);
    __s_->__next_ = std::addressof(__s_->__own_);
  }
  _LIBCPP_HIDE_FROM_ABI tls_stream(any_stream& __stream, const tls_context& __ctx) : __s_(__make(__ctx)) {
    __s_->__next_ = std::addressof(__stream);
  }
  tls_stream(const tls_stream&)                                      = delete;
  tls_stream& operator=(const tls_stream&)                           = delete;
  _LIBCPP_HIDE_FROM_ABI tls_stream(tls_stream&&) noexcept            = default;
  _LIBCPP_HIDE_FROM_ABI tls_stream& operator=(tls_stream&&) noexcept = default;
  _LIBCPP_HIDE_FROM_ABI ~tls_stream()                                = default;

  // The host name for SNI and certificate verification; used by the next client handshake. An IP literal is matched
  // against the iPAddress entries of the certificate and sends no SNI; empty (the default) disables both. The names are
  // checked only when the verify mode of the context is not none.
  _LIBCPP_HIDE_FROM_ABI void set_hostname(string_view __host) { __s_->__host_.assign(__host); }

  // Performs the handshake (each call starts a new session). The wrapped stream must be connected.
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI task<io_result<>> handshake(tls_role __role) { return __s_->__handshake(__role); }

  template <MutableBufferSequence _Seq>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI task<io_result<size_t>> read_some(_Seq __seq) {
    return __s_->template __read_some<_Seq>(std::move(__seq));
  }
  template <ConstBufferSequence _Seq>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI task<io_result<size_t>> write_some(_Seq __seq) {
    return __s_->template __write_some<_Seq>(std::move(__seq));
  }

  // Sends the close_notify and waits for that of the peer (io::error::stream_truncated if the wrapped stream ends
  // first). The transport is not closed.
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI task<io_result<>> shutdown() { return __s_->__shutdown(); }

  // The protocol selected by ALPN, empty if there is none or no handshake has completed.
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI string_view alpn_protocol() const noexcept {
    return __s_->__established_ && __s_->__eng_ ? __s_->__eng_->__alpn() : string_view();
  }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI bool is_established() const noexcept { return __s_->__established_; }
  // The text the engine gave for the last failure (the OpenSSL error strings), empty if there was none.
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI string error_detail() const { return __s_->__detail_; }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI any_stream& next_layer() noexcept { return *__s_->__next_; }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI string_view name() const noexcept { return "openssl"; }
};

} // namespace io

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP_HAS_IO_TLS

#endif // _LIBCPP___NET_TLS_TLS_STREAM_H
