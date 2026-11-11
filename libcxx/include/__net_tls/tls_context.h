//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef _LIBCPP___NET_TLS_TLS_CONTEXT_H
#define _LIBCPP___NET_TLS_TLS_CONTEXT_H

#include <__config>
#include <__io/support/config.h>
#include <__memory/shared_ptr.h>
#include <__net_tls/engine.h>
#include <__net_tls/error.h>
#include <__net_tls/support/openssl_dl.h>
#include <__system_error/errc.h>
#include <__system_error/error_code.h>
#include <__utility/move.h>
#include <cerrno>
#include <cstdio>
#include <initializer_list>
#include <string>
#include <string_view>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_IO_TLS

_LIBCPP_PUSH_MACROS
#  include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

// Whether the cryptographic engine (OpenSSL, loaded at run time) is available. If it is not, every TLS operation
// completes with tls_error::engine_unavailable.
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI inline bool tls_engine_available() noexcept { return io::__ossl_get() != nullptr; }

_LIBCPP_HIDE_FROM_ABI inline error_code __tls_read_file(string_view __path, string& __dst) {
  string __p(__path);
  FILE* __f = std::fopen(__p.c_str(), "rb");
  if (!__f)
    return error_code(errno ? errno : EIO, generic_category());
  char __buf[4096];
  size_t __n;
  while ((__n = std::fread(__buf, 1, sizeof(__buf), __f)) != 0)
    __dst.append(__buf, __n);
  bool __bad = std::ferror(__f) != 0;
  std::fclose(__f);
  return __bad ? error_code(EIO, generic_category()) : error_code();
}

// P4100R1 [8.10]; provisional specification: Corosio's tls_context. The portable configuration of TLS connections:
// credentials, trust anchors, verification, protocol window, cipher suites and ALPN. A tls_context is a shared handle
// to its state (copies share it); the settings are recorded here and applied to the engine when the first stream is
// created from the context, so a context must not be modified once a stream has been created from it (later changes do
// not apply). Setters return an error_code for the faults that can be seen immediately (an unreadable file, an invalid
// argument); a malformed certificate, key or cipher string surfaces as tls_error::bad_configuration when the first
// handshake builds the native context. The context is not thread-safe while it is being modified.
class tls_context {
  shared_ptr<__tls_ctx_data> __d_;

public:
  _LIBCPP_HIDE_FROM_ABI tls_context() : __d_(make_shared<__tls_ctx_data>()) {}
  _LIBCPP_HIDE_FROM_ABI tls_context(const tls_context&)                = default;
  _LIBCPP_HIDE_FROM_ABI tls_context& operator=(const tls_context&)     = default;
  _LIBCPP_HIDE_FROM_ABI tls_context(tls_context&&) noexcept            = default;
  _LIBCPP_HIDE_FROM_ABI tls_context& operator=(tls_context&&) noexcept = default;
  _LIBCPP_HIDE_FROM_ABI ~tls_context()                                 = default;

  // The entity certificate (for a server, its certificate; for a client using mutual TLS, the client certificate), PEM.
  // use_certificate_chain() takes the certificate followed by the intermediate certificates, leaf first.
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI error_code use_certificate(string_view __pem) {
    __d_->__cfg_.__certificate_pem_.assign(__pem);
    return error_code();
  }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI error_code use_certificate_chain(string_view __pem) {
    return use_certificate(__pem);
  }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI error_code use_certificate_file(string_view __path) {
    string __s;
    error_code __ec = io::__tls_read_file(__path, __s);
    if (!__ec)
      __d_->__cfg_.__certificate_pem_ = std::move(__s);
    return __ec;
  }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI error_code use_certificate_chain_file(string_view __path) {
    return use_certificate_file(__path);
  }

  // The private key of the certificate, PEM. An encrypted key is rejected (there is no password callback).
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI error_code use_private_key(string_view __pem) {
    __d_->__cfg_.__private_key_pem_.assign(__pem);
    return error_code();
  }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI error_code use_private_key_file(string_view __path) {
    string __s;
    error_code __ec = io::__tls_read_file(__path, __s);
    if (!__ec)
      __d_->__cfg_.__private_key_pem_ = std::move(__s);
    return __ec;
  }

  // Trust anchors: certificates (PEM, any number) from memory or a file, a directory of hashed certificates (as made by
  // `openssl rehash`), or the default store of the engine (the CA file and directory OpenSSL was built with, or named
  // by SSL_CERT_FILE and SSL_CERT_DIR; not the Keychain or the Windows store).
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI error_code add_certificate_authority(string_view __pem) {
    __d_->__cfg_.__trust_anchors_pem_.append(__pem);
    if (!__pem.empty() && __pem.back() != '\n')
      __d_->__cfg_.__trust_anchors_pem_.push_back('\n');
    return error_code();
  }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI error_code load_verify_file(string_view __path) {
    string __s;
    error_code __ec = io::__tls_read_file(__path, __s);
    return __ec ? __ec : add_certificate_authority(__s);
  }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI error_code add_verify_path(string_view __path) {
    __d_->__cfg_.__verify_paths_.emplace_back(__path);
    return error_code();
  }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI error_code set_default_verify_paths() {
    __d_->__cfg_.__default_verify_paths_ = true;
    return error_code();
  }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI error_code use_system_trust() { return set_default_verify_paths(); }

  // The protocol window; the default minimum is TLS 1.2 and the default maximum the newest version.
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI error_code set_min_protocol_version(tls_version __v) {
    __d_->__cfg_.__min_version_ = __v;
    return error_code();
  }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI error_code set_max_protocol_version(tls_version __v) {
    __d_->__cfg_.__max_version_ = __v;
    return error_code();
  }

  // The cipher list of TLS 1.2 and older (OpenSSL syntax) and the colon-separated suites of TLS 1.3.
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI error_code set_ciphersuites(string_view __list) {
    __d_->__cfg_.__ciphers_.assign(__list);
    return error_code();
  }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI error_code set_ciphersuites_tls13(string_view __list) {
    __d_->__cfg_.__ciphersuites_tls13_.assign(__list);
    return error_code();
  }

  // ALPN: the protocols in order of preference; a protocol name has 1 to 255 bytes. A server selects the first of its
  // protocols that the client offers and fails the handshake if there is none in common; a client offers the list.
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI error_code set_alpn(initializer_list<string_view> __protocols) {
    vector<string> __list;
    string __wire;
    for (string_view __p : __protocols) {
      if (__p.empty() || __p.size() > 255)
        return make_error_code(errc::invalid_argument);
      __list.emplace_back(__p);
      __wire.push_back(static_cast<char>(__p.size()));
      __wire.append(__p);
    }
    __d_->__cfg_.__alpn_      = std::move(__list);
    __d_->__cfg_.__alpn_wire_ = std::move(__wire);
    return error_code();
  }

  // Verification of the peer certificate. The default is tls_verify_mode::peer (Corosio defaults to none, which accepts
  // any certificate; a deviation for a secure default): a client verifies the server certificate, a server requests but
  // does not require a client certificate. Pass tls_verify_mode::none to accept any certificate.
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI error_code set_verify_mode(tls_verify_mode __m) {
    __d_->__cfg_.__verify_ = __m;
    return error_code();
  }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI error_code set_verify_depth(int __depth) {
    if (__depth < 0)
      return make_error_code(errc::invalid_argument);
    __d_->__cfg_.__verify_depth_ = __depth;
    return error_code();
  }

  // Implementation detail: the shared state, for tls_stream.
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI const shared_ptr<__tls_ctx_data>& __data() const noexcept { return __d_; }
};

} // namespace io

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP_HAS_IO_TLS

#endif // _LIBCPP___NET_TLS_TLS_CONTEXT_H
