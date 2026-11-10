//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef _LIBCPP___NET_TLS_ERROR_H
#define _LIBCPP___NET_TLS_ERROR_H

#include <__config>
#include <__io/support/config.h>
#include <__system_error/error_category.h>
#include <__system_error/error_code.h>
#include <__system_error/error_condition.h>
#include <__type_traits/integral_constant.h>
#include <string>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_IO_TLS

_LIBCPP_PUSH_MACROS
#  include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

// P4100R1 [8.10]; provisional specification: Corosio's tls_stream and tls_context (no WG21 paper yet).

// Which end of the handshake a stream plays.
enum class tls_role { client, server };

enum class tls_version { tls_1_2, tls_1_3 };

// How the certificate of the peer is verified: none (accept anything), peer (verify what the peer presents; a server
// requests a client certificate but accepts a client without one), require_peer (a server rejects a client without a
// certificate; for a client the same as peer).
enum class tls_verify_mode { none, peer, require_peer };

// The errors of the TLS layer (category tls_category()). Errors of the wrapped stream are passed through unchanged, and
// the end of the stream is reported as io::error::eof (a close_notify of the peer) or io::error::stream_truncated (the
// transport ended without one).
enum class tls_error {
  engine_unavailable = 1, // the cryptographic engine (OpenSSL) could not be loaded
  bad_configuration,      // a certificate, key, trust anchor, cipher list or protocol window of the context is unusable
  handshake_failed,       // the handshake failed for a reason other than a certificate
  certificate_verify_failed, // the certificate of the peer was rejected (untrusted, expired, wrong host name, ...)
  protocol_error,            // a TLS protocol violation or a fatal alert after the handshake
  not_established            // the operation needs a completed handshake
};

_LIBCPP_DIAGNOSTIC_PUSH
_LIBCPP_CLANG_DIAGNOSTIC_IGNORED("-Wweak-vtables")
class __tls_category_t final : public error_category {
public:
  _LIBCPP_HIDE_FROM_ABI __tls_category_t() noexcept = default;
  _LIBCPP_HIDE_FROM_ABI_VIRTUAL const char* name() const noexcept override { return "std::io::tls"; }
  // The text of the code; the detail that the engine reports for a particular failure (the OpenSSL error strings) is
  // available from tls_stream::error_detail().
  _LIBCPP_HIDE_FROM_ABI_VIRTUAL string message(int __ev) const override {
    switch (static_cast<tls_error>(__ev)) {
    case tls_error::engine_unavailable:
      return "the TLS engine (OpenSSL) is not available";
    case tls_error::bad_configuration:
      return "unusable TLS configuration (certificate, key, trust anchor, cipher list or protocol version)";
    case tls_error::handshake_failed:
      return "TLS handshake failed";
    case tls_error::certificate_verify_failed:
      return "TLS certificate verification failed";
    case tls_error::protocol_error:
      return "TLS protocol error";
    case tls_error::not_established:
      return "the TLS handshake has not completed";
    }
    return "unknown std::io TLS error";
  }
};
_LIBCPP_DIAGNOSTIC_POP

_LIBCPP_HIDE_FROM_ABI inline const error_category& tls_category() noexcept {
  static const __tls_category_t __c;
  return __c;
}

_LIBCPP_HIDE_FROM_ABI inline error_code make_error_code(tls_error __e) noexcept {
  return error_code(static_cast<int>(__e), tls_category());
}

} // namespace io

template <>
struct is_error_code_enum<io::tls_error> : true_type {};

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP_HAS_IO_TLS

#endif // _LIBCPP___NET_TLS_ERROR_H
