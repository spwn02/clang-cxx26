//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef _LIBCPP___NET_TLS_OPENSSL_ENGINE_H
#define _LIBCPP___NET_TLS_OPENSSL_ENGINE_H

#include <__config>
#include <__io/support/config.h>
#include <__memory/shared_ptr.h>
#include <__mutex/lock_guard.h>
#include <__net_tls/engine.h>
#include <__net_tls/error.h>
#include <__net_tls/support/openssl_dl.h>
#include <__system_error/error_code.h>
#include <__utility/move.h>
#include <climits>
#include <cstddef>
#include <cstring>
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

// The record size of TLS: one write is limited to it, so that a large write never queues megabytes of ciphertext.
inline constexpr size_t __tls_record_size = 16384;

_LIBCPP_HIDE_FROM_ABI inline bool __tls_is_ip_literal(string_view __s) noexcept {
  if (__s.find(':') != string_view::npos)
    return true; // IPv6
  int __parts = 0, __digits = 0;
  for (char __c : __s) {
    if (__c == '.') {
      if (__digits == 0)
        return false;
      ++__parts;
      __digits = 0;
    } else if (__c >= '0' && __c <= '9') {
      if (++__digits > 3)
        return false;
    } else {
      return false;
    }
  }
  return __parts == 3 && __digits != 0;
}

// Collects the OpenSSL error queue of the calling thread into one string and returns the first code.
_LIBCPP_HIDE_FROM_ABI inline unsigned long __ossl_drain_errors(const __ossl_api& __a, string& __detail) {
  unsigned long __first = 0;
  while (unsigned long __e = __a.__ERR_get_error()) {
    if (__first == 0)
      __first = __e;
    char __buf[256];
    __a.__ERR_error_string_n(__e, __buf, sizeof(__buf));
    if (!__detail.empty())
      __detail += "; ";
    __detail += __buf;
  }
  return __first;
}

// The password callback for private keys: there is no way to ask for a password, so an encrypted key is rejected
// instead of prompting on the terminal.
_LIBCPP_HIDE_FROM_ABI inline int __ossl_no_password(char*, int, int, void*) { return 0; }

// Server side ALPN: the first protocol of the server list that the client also offers (the server's preference wins);
// no overlap is a fatal no_application_protocol alert (RFC 7301).
_LIBCPP_HIDE_FROM_ABI inline int __ossl_alpn_select(
    __ossl_ssl*, const unsigned char** __selected, unsigned char* __outlen, const unsigned char* __offered, unsigned int __inlen,
    void* __arg) {
  const __ossl_api* __a = io::__ossl_get();
  auto* __wire          = static_cast<const string*>(__arg);
  if (!__a || !__wire)
    return __ossl_tlsext_err_alert_fatal;
  unsigned char* __sel    = nullptr;
  unsigned char __sel_len = 0;
  int __r                 = __a->__SSL_select_next_proto(
      &__sel,
      &__sel_len,
      reinterpret_cast<const unsigned char*>(__wire->data()),
      static_cast<unsigned int>(__wire->size()),
      __offered,
      __inlen);
  if (__r != __ossl_npn_negotiated)
    return __ossl_tlsext_err_alert_fatal;
  *__selected = __sel;
  *__outlen = __sel_len;
  return __ossl_tlsext_err_ok;
}

_LIBCPP_HIDE_FROM_ABI inline error_code __ossl_fail(error_code& __ec, string& __detail, tls_error __e, string __d) {
  __ec     = make_error_code(__e);
  __detail = std::move(__d);
  return __ec;
}

// Reads every PEM certificate of the text; the callback receives each one (and its ownership).
template <class _Fn>
_LIBCPP_HIDE_FROM_ABI bool __ossl_for_each_certificate(const __ossl_api& __a, const string& __pem, _Fn __fn) {
  __ossl_bio* __bio = __a.__BIO_new_mem_buf(__pem.data(), static_cast<int>(__pem.size()));
  if (!__bio)
    return false;
  size_t __expected = 0; // a certificate that cannot be parsed makes the whole text unusable
  for (size_t __p = __pem.find("-----BEGIN"); __p != string::npos; __p = __pem.find("-----BEGIN", __p + 1))
    ++__expected;
  size_t __parsed = 0;
  bool __any      = false;
  while (__ossl_x509* __x = __a.__PEM_read_bio_X509(__bio, nullptr, nullptr, nullptr)) {
    __any = true;
    ++__parsed;
    __fn(__x);
    __a.__X509_free(__x);
  }
  __a.__BIO_free(__bio);
  __a.__ERR_clear_error(); // the end of the text is reported as an error
  return __any && __parsed == __expected;
}

// Builds the native context (an SSL_CTX) of a tls_context once; the result, or the failure, is cached in the data.
_LIBCPP_HIDE_FROM_ABI inline void __ossl_build_native(__tls_ctx_data& __d, const __ossl_api& __a) {
  lock_guard<mutex> __lk(__d.__mutex_);
  if (__d.__built_)
    return;
  __d.__built_            = true;
  const __tls_config& __c = __d.__cfg_;
  const __ossl_api* __ap  = std::addressof(__a);
  __ossl_ctx* __ctx       = __a.__SSL_CTX_new(__a.__TLS_method());
  if (!__ctx) {
    io::__ossl_drain_errors(__a, __d.__build_detail_);
    __d.__build_error_ = make_error_code(tls_error::bad_configuration);
    return;
  }
  __d.__native_ = shared_ptr<void>(__ctx, [__ap](void* __p) { __ap->__SSL_CTX_free(static_cast<__ossl_ctx*>(__p)); });

  auto __bad = [&](string __why) {
    string __more;
    io::__ossl_drain_errors(__a, __more);
    __d.__build_error_  = make_error_code(tls_error::bad_configuration);
    __d.__build_detail_ = std::move(__why);
    if (!__more.empty()) {
      __d.__build_detail_ += ": ";
      __d.__build_detail_ += __more;
    }
    __d.__native_.reset();
  };

  __a.__SSL_CTX_ctrl(
      __ctx, __ossl_ctrl_mode, __ossl_mode_enable_partial_write | __ossl_mode_accept_moving_write_buffer, nullptr);
  if (__c.__min_version_ > __c.__max_version_)
    return __bad("the minimum protocol version is newer than the maximum");
  auto __ver = [](tls_version __v) { return __v == tls_version::tls_1_3 ? __ossl_tls1_3_version : __ossl_tls1_2_version; };
  if (__a.__SSL_CTX_ctrl(__ctx, __ossl_ctrl_set_min_proto_version, __ver(__c.__min_version_), nullptr) != 1 ||
      __a.__SSL_CTX_ctrl(__ctx, __ossl_ctrl_set_max_proto_version, __ver(__c.__max_version_), nullptr) != 1)
    return __bad("cannot set the protocol versions");

  int __vf = __ossl_ssl_verify_none;
  if (__c.__verify_ == tls_verify_mode::peer)
    __vf = __ossl_ssl_verify_peer;
  else if (__c.__verify_ == tls_verify_mode::require_peer)
    __vf = __ossl_ssl_verify_peer | __ossl_ssl_verify_fail_if_no_peer_cert;
  __a.__SSL_CTX_set_verify(__ctx, __vf, nullptr);
  if (__c.__verify_depth_ >= 0)
    __a.__SSL_CTX_set_verify_depth(__ctx, __c.__verify_depth_);

  if (!__c.__certificate_pem_.empty()) {
    bool __first = true, __failed = false;
    bool __found = io::__ossl_for_each_certificate(__a, __c.__certificate_pem_, [&](__ossl_x509* __x) {
      if (__first) {
        __first = false;
        if (__a.__SSL_CTX_use_certificate(__ctx, __x) != 1)
          __failed = true;
      } else if (__a.__SSL_CTX_ctrl(__ctx, __ossl_ctrl_chain_cert, 1, __x) != 1) {
        __failed = true;
      }
    });
    if (!__found || __failed)
      return __bad("cannot use the certificate chain");
  }
  if (!__c.__private_key_pem_.empty()) {
    __ossl_bio* __bio = __a.__BIO_new_mem_buf(__c.__private_key_pem_.data(), static_cast<int>(__c.__private_key_pem_.size()));
    __ossl_pkey* __k  = __bio ? __a.__PEM_read_bio_PrivateKey(
                                   __bio, nullptr, reinterpret_cast<void*>(&io::__ossl_no_password), nullptr)
                              : nullptr;
    if (__bio)
      __a.__BIO_free(__bio);
    if (!__k)
      return __bad("cannot read the private key");
    int __ok = __a.__SSL_CTX_use_PrivateKey(__ctx, __k);
    __a.__EVP_PKEY_free(__k);
    if (__ok != 1)
      return __bad("cannot use the private key");
  }
  if (!__c.__certificate_pem_.empty() && !__c.__private_key_pem_.empty() &&
      __a.__SSL_CTX_check_private_key(__ctx) != 1)
    return __bad("the private key does not match the certificate");

  if (!__c.__trust_anchors_pem_.empty()) {
    __ossl_store* __store = __a.__SSL_CTX_get_cert_store(__ctx);
    bool __failed         = false;
    bool __found          = io::__ossl_for_each_certificate(__a, __c.__trust_anchors_pem_, [&](__ossl_x509* __x) {
      if (__a.__X509_STORE_add_cert(__store, __x) != 1) {
        // A certificate that is already in the store is not an error.
        string __ignored;
        unsigned long __e = io::__ossl_drain_errors(__a, __ignored);
        if (__e != 0 && (__e & 0x7FFFFF) != 101 /* X509_R_CERT_ALREADY_IN_HASH_TABLE */)
          __failed = true;
      }
    });
    if (!__found || __failed)
      return __bad("cannot use the trust anchors");
  }
  for (const string& __p : __c.__verify_paths_)
    if (__a.__SSL_CTX_load_verify_locations(__ctx, nullptr, __p.c_str()) != 1)
      return __bad(string("cannot use the verify path ").append(__p));
  if (__c.__default_verify_paths_)
    (void)__a.__SSL_CTX_set_default_verify_paths(__ctx); // a store that cannot be loaded is skipped, as in Corosio

  if (!__c.__ciphers_.empty() && __a.__SSL_CTX_set_cipher_list(__ctx, __c.__ciphers_.c_str()) != 1)
    return __bad("invalid cipher list");
  if (!__c.__ciphersuites_tls13_.empty() && __a.__SSL_CTX_set_ciphersuites(__ctx, __c.__ciphersuites_tls13_.c_str()) != 1)
    return __bad("invalid TLS 1.3 cipher suites");
  if (!__c.__alpn_wire_.empty())
    __a.__SSL_CTX_set_alpn_select_cb(__ctx, &io::__ossl_alpn_select, const_cast<string*>(&__c.__alpn_wire_));
  io::__ossl_drain_errors(__a, __d.__build_detail_);
  __d.__build_detail_.clear();
}

_LIBCPP_DIAGNOSTIC_PUSH
_LIBCPP_CLANG_DIAGNOSTIC_IGNORED("-Wweak-vtables")
class __openssl_engine final : public __tls_engine {
  shared_ptr<__tls_ctx_data> __data_;
  const __ossl_api* __api_ = nullptr;
  __ossl_ssl* __ssl_       = nullptr;
  __ossl_bio* __rbio_      = nullptr; // owned by the session after __start
  __ossl_bio* __wbio_      = nullptr;
  error_code __ec_;
  string __detail_;
  string __alpn_;
  bool __handshaking_ = true;

  _LIBCPP_HIDE_FROM_ABI __tls_io __classify(int __ret) noexcept {
    const __ossl_api& __a = *__api_;
    int __err             = __a.__SSL_get_error(__ssl_, __ret);
    switch (__err) {
    case __ossl_ssl_error_want_read:
    case __ossl_ssl_error_want_write:
      __a.__ERR_clear_error();
      return __tls_io::__want_read;
    case __ossl_ssl_error_zero_return:
      __a.__ERR_clear_error();
      return __tls_io::__closed;
    default:
      break;
    }
    string __d;
    unsigned long __first = io::__ossl_drain_errors(__a, __d);
    long __vr             = __a.__SSL_get_verify_result(__ssl_);
    unsigned __lib        = static_cast<unsigned>((__first >> 23) & 0xFF);
    unsigned __reason     = static_cast<unsigned>(__first & 0x7FFFFF);
    // The alerts of the peer that mean it rejected our certificate (1000 + the alert number: bad_certificate 42,
    // certificate_revoked 44, certificate_expired 45, certificate_unknown 46, unknown_ca 48).
    bool __peer_rejected_cert = __lib == 20 && (__reason == 1042 || __reason == 1044 || __reason == 1045 ||
                                                __reason == 1046 || __reason == 1048);
    tls_error __code;
    if (__vr != __ossl_x509_v_ok) {
      __code = tls_error::certificate_verify_failed;
      const char* __s = __a.__X509_verify_cert_error_string(__vr);
      string __msg(__s ? __s : "certificate verification failed");
      if (!__d.empty()) {
        __msg += "; ";
        __msg += __d;
      }
      __d = std::move(__msg);
    } else if (__peer_rejected_cert) {
      __code = tls_error::certificate_verify_failed;
    } else {
      __code = __handshaking_ ? tls_error::handshake_failed : tls_error::protocol_error;
    }
    if (__d.empty())
      __d = __err == __ossl_ssl_error_syscall ? "unexpected end of data" : "TLS failure";
    io::__ossl_fail(__ec_, __detail_, __code, std::move(__d));
    return __tls_io::__error;
  }

public:
  _LIBCPP_HIDE_FROM_ABI explicit __openssl_engine(shared_ptr<__tls_ctx_data> __d) : __data_(std::move(__d)) {}
  __openssl_engine(const __openssl_engine&)            = delete;
  __openssl_engine& operator=(const __openssl_engine&) = delete;
  _LIBCPP_HIDE_FROM_ABI_VIRTUAL ~__openssl_engine() override {
    if (__ssl_)
      __api_->__SSL_free(__ssl_); // frees the memory BIOs as well
  }

  _LIBCPP_HIDE_FROM_ABI_VIRTUAL error_code __start(tls_role __role, string_view __hostname) noexcept override {
    __api_ = io::__ossl_get();
    if (!__api_)
      return io::__ossl_fail(__ec_, __detail_, tls_error::engine_unavailable, "libssl could not be loaded");
    const __ossl_api& __a = *__api_;
    io::__ossl_build_native(*__data_, __a);
    if (__data_->__build_error_)
      return io::__ossl_fail(__ec_, __detail_, tls_error::bad_configuration, __data_->__build_detail_);
    __ssl_ = __a.__SSL_new(static_cast<__ossl_ctx*>(__data_->__native_.get()));
    if (__ssl_) {
      __rbio_ = __a.__BIO_new(__a.__BIO_s_mem());
      __wbio_ = __a.__BIO_new(__a.__BIO_s_mem());
    }
    if (!__ssl_ || !__rbio_ || !__wbio_) {
      if (__rbio_)
        __a.__BIO_free(__rbio_);
      if (__wbio_)
        __a.__BIO_free(__wbio_);
      if (__ssl_)
        __a.__SSL_free(__ssl_);
      __ssl_ = nullptr;
      string __d;
      io::__ossl_drain_errors(__a, __d);
      return io::__ossl_fail(__ec_, __detail_, tls_error::bad_configuration, string("cannot create the TLS session ").append(__d));
    }
    __a.__SSL_set_bio(__ssl_, __rbio_, __wbio_);
    if (__role == tls_role::client) {
      __a.__SSL_set_connect_state(__ssl_);
      if (!__hostname.empty()) {
        string __name(__hostname);
        __ossl_vparam* __param = __a.__SSL_get0_param(__ssl_);
        bool __ok              = __param != nullptr;
        if (__ok && io::__tls_is_ip_literal(__hostname)) {
          // RFC 6066: no SNI for an address; it is matched against the iPAddress entries of the certificate.
          __ok = __a.__X509_VERIFY_PARAM_set1_ip_asc(__param, __name.c_str()) == 1;
        } else if (__ok) {
          __ok = __a.__SSL_ctrl(
                     __ssl_, __ossl_ctrl_set_tlsext_hostname, __ossl_tlsext_nametype_host_name, const_cast<char*>(__name.c_str())) == 1 &&
                 __a.__X509_VERIFY_PARAM_set1_host(__param, __name.c_str(), 0) == 1;
        }
        if (!__ok)
          return io::__ossl_fail(__ec_, __detail_, tls_error::bad_configuration, "cannot set the host name");
      }
      const string& __wire = __data_->__cfg_.__alpn_wire_;
      if (!__wire.empty() &&
          __a.__SSL_set_alpn_protos(
              __ssl_, reinterpret_cast<const unsigned char*>(__wire.data()), static_cast<unsigned>(__wire.size())) != 0)
        return io::__ossl_fail(__ec_, __detail_, tls_error::bad_configuration, "cannot set the ALPN protocols");
    } else {
      __a.__SSL_set_accept_state(__ssl_);
    }
    return error_code();
  }

  _LIBCPP_HIDE_FROM_ABI_VIRTUAL __tls_io __handshake() noexcept override {
    const __ossl_api& __a = *__api_;
    __a.__ERR_clear_error();
    int __r = __a.__SSL_do_handshake(__ssl_);
    if (__r == 1) {
      __handshaking_                  = false;
      const unsigned char* __proto    = nullptr;
      unsigned int __len              = 0;
      __a.__SSL_get0_alpn_selected(__ssl_, &__proto, &__len);
      if (__proto)
        __alpn_.assign(reinterpret_cast<const char*>(__proto), __len);
      return __tls_io::__ok;
    }
    __tls_io __s = __classify(__r);
    return __s == __tls_io::__closed ? (io::__ossl_fail(__ec_, __detail_, tls_error::handshake_failed, "closed during the handshake"), __tls_io::__error) : __s;
  }

  _LIBCPP_HIDE_FROM_ABI_VIRTUAL __tls_io __read(void* __buf, size_t __n, size_t& __got) noexcept override {
    const __ossl_api& __a = *__api_;
    __got                 = 0;
    __a.__ERR_clear_error();
    int __r = __a.__SSL_read(__ssl_, __buf, __n > static_cast<size_t>(INT_MAX) ? INT_MAX : static_cast<int>(__n));
    if (__r > 0) {
      __got = static_cast<size_t>(__r);
      return __tls_io::__ok;
    }
    return __classify(__r);
  }

  _LIBCPP_HIDE_FROM_ABI_VIRTUAL __tls_io __write(const void* __buf, size_t __n, size_t& __put) noexcept override {
    const __ossl_api& __a = *__api_;
    __put                 = 0;
    __a.__ERR_clear_error();
    int __r = __a.__SSL_write(__ssl_, __buf, __n > __tls_record_size ? static_cast<int>(__tls_record_size) : static_cast<int>(__n));
    if (__r > 0) {
      __put = static_cast<size_t>(__r);
      return __tls_io::__ok;
    }
    __tls_io __s = __classify(__r);
    if (__s == __tls_io::__want_read) { // a renegotiation request: not supported
      io::__ossl_fail(__ec_, __detail_, tls_error::protocol_error, "renegotiation is not supported");
      return __tls_io::__error;
    }
    return __s;
  }

  _LIBCPP_HIDE_FROM_ABI_VIRTUAL __tls_io __shutdown() noexcept override {
    const __ossl_api& __a = *__api_;
    __a.__ERR_clear_error();
    int __r = __a.__SSL_shutdown(__ssl_);
    if (__r == 1)
      return __tls_io::__ok;
    if (__r == 0)
      return __tls_io::__want_read;
    __tls_io __s = __classify(__r);
    return __s == __tls_io::__closed ? __tls_io::__ok : __s;
  }

  _LIBCPP_HIDE_FROM_ABI_VIRTUAL void __feed(const void* __p, size_t __n) noexcept override {
    const char* __c = static_cast<const char*>(__p);
    while (__n != 0) {
      int __k = __n > static_cast<size_t>(INT_MAX) ? INT_MAX : static_cast<int>(__n);
      int __w = __api_->__BIO_write(__rbio_, __c, __k);
      if (__w <= 0)
        return;
      __c += __w;
      __n -= static_cast<size_t>(__w);
    }
  }

  _LIBCPP_HIDE_FROM_ABI_VIRTUAL size_t __take_output(void* __p, size_t __n) noexcept override {
    long __pending = __api_->__BIO_ctrl(__wbio_, __ossl_bio_ctrl_pending, 0, nullptr);
    if (__pending <= 0)
      return 0;
    size_t __k = static_cast<size_t>(__pending) < __n ? static_cast<size_t>(__pending) : __n;
    int __r    = __api_->__BIO_read(__wbio_, __p, static_cast<int>(__k));
    return __r > 0 ? static_cast<size_t>(__r) : 0;
  }

  _LIBCPP_HIDE_FROM_ABI_VIRTUAL error_code __error() const noexcept override { return __ec_; }
  _LIBCPP_HIDE_FROM_ABI_VIRTUAL string __error_detail() const override { return __detail_; }
  _LIBCPP_HIDE_FROM_ABI_VIRTUAL string_view __alpn() const noexcept override { return __alpn_; }
};
_LIBCPP_DIAGNOSTIC_POP

} // namespace io

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP_HAS_IO_TLS

#endif // _LIBCPP___NET_TLS_OPENSSL_ENGINE_H
