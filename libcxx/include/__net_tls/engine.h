//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef _LIBCPP___NET_TLS_ENGINE_H
#define _LIBCPP___NET_TLS_ENGINE_H

#include <__config>
#include <__io/support/config.h>
#include <__memory/shared_ptr.h>
#include <__memory/unique_ptr.h>
#include <__mutex/mutex.h>
#include <__net_tls/error.h>
#include <__system_error/error_code.h>
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

// The configuration a tls_context records; an engine turns it into its native context when the first stream is created.
struct __tls_config {
  string __certificate_pem_; // the entity certificate, followed by the intermediate certificates
  string __private_key_pem_;
  string __trust_anchors_pem_;
  vector<string> __verify_paths_;
  bool __default_verify_paths_ = false;
  tls_version __min_version_   = tls_version::tls_1_2;
  tls_version __max_version_   = tls_version::tls_1_3;
  tls_verify_mode __verify_    = tls_verify_mode::peer;
  int __verify_depth_          = -1;
  string __ciphers_;
  string __ciphersuites_tls13_;
  vector<string> __alpn_;
  string __alpn_wire_; // the same list, length-prefixed
};

// The shared state of a tls_context: the configuration and the native context built from it, once, by the engine.
struct __tls_ctx_data {
  __tls_config __cfg_;
  mutex __mutex_;
  bool __built_ = false;
  shared_ptr<void> __native_;
  error_code __build_error_;
  string __build_detail_;
};

enum class __tls_io {
  __ok,        // the operation made progress / completed
  __want_read, // needs ciphertext from the transport
  __closed,    // the peer's close_notify was received
  __error      // see __error() and __error_detail()
};

// The interface between the stream and a cryptographic engine (OpenSSL through dlopen today; Secure Transport, SChannel or
// a test double later). The engine is driven by memory buffers: the stream gives it ciphertext read from the transport
// with __feed() and takes the ciphertext it wants to send with __take_output(); it never touches the transport. Nothing
// here blocks. The stream serializes the calls.
_LIBCPP_DIAGNOSTIC_PUSH
_LIBCPP_CLANG_DIAGNOSTIC_IGNORED("-Wweak-vtables")
class __tls_engine {
public:
  _LIBCPP_HIDE_FROM_ABI_VIRTUAL virtual ~__tls_engine() = default;

  // Creates the session for the role; the host name (client only) is used for SNI and certificate verification.
  virtual error_code __start(tls_role, string_view __hostname) noexcept = 0;
  virtual __tls_io __handshake() noexcept                               = 0;
  virtual __tls_io __read(void*, size_t, size_t& __got) noexcept        = 0;
  virtual __tls_io __write(const void*, size_t, size_t& __put) noexcept = 0;
  // Sends our close_notify; __ok once the peer's has also been received, __want_read while it is outstanding.
  virtual __tls_io __shutdown() noexcept = 0;

  virtual void __feed(const void*, size_t) noexcept         = 0;
  virtual size_t __take_output(void*, size_t) noexcept      = 0;
  virtual error_code __error() const noexcept               = 0;
  virtual string __error_detail() const                     = 0;
  virtual string_view __alpn() const noexcept               = 0;
};
_LIBCPP_DIAGNOSTIC_POP

} // namespace io

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP_HAS_IO_TLS

#endif // _LIBCPP___NET_TLS_ENGINE_H
