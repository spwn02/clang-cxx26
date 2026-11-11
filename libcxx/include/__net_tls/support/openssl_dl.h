//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef _LIBCPP___NET_TLS_SUPPORT_OPENSSL_DL_H
#define _LIBCPP___NET_TLS_SUPPORT_OPENSSL_DL_H

#include <__config>
#include <__io/support/config.h>

#if _LIBCPP_HAS_IO_TLS
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
#  include <atomic>
#  include <dlfcn.h>
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

#  if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#    pragma GCC system_header
#  endif

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

// The OpenSSL loader of <net_tls>. The library has no build or link dependency on OpenSSL and includes none of its
// headers: the few entry points it needs are declared here (opaque types, function types with the OpenSSL 3 signatures,
// the numeric constants) and resolved with dlopen/dlsym the first time a TLS facility is used. If libssl cannot be
// loaded, __ossl_get() returns nullptr and every TLS operation reports tls_error::engine_unavailable.
//
// This is the only place that knows about the shared library; a different engine (Secure Transport, SChannel, a test
// double) implements the same __tls_engine interface (see __net_tls/engine.h) without touching it.

struct __ossl_ssl;
struct __ossl_ctx;
struct __ossl_method;
struct __ossl_bio;
struct __ossl_bio_method;
struct __ossl_x509;
struct __ossl_pkey;
struct __ossl_store;
struct __ossl_vparam;

// Constants, from <openssl/ssl.h>, <openssl/bio.h>, <openssl/tls1.h> and <openssl/x509_vfy.h> of OpenSSL 3.
inline constexpr int __ossl_ssl_verify_none                  = 0x00;
inline constexpr int __ossl_ssl_verify_peer                  = 0x01;
inline constexpr int __ossl_ssl_verify_fail_if_no_peer_cert  = 0x02;
inline constexpr int __ossl_ssl_error_none                   = 0;
inline constexpr int __ossl_ssl_error_ssl                    = 1;
inline constexpr int __ossl_ssl_error_want_read              = 2;
inline constexpr int __ossl_ssl_error_want_write             = 3;
inline constexpr int __ossl_ssl_error_syscall                = 5;
inline constexpr int __ossl_ssl_error_zero_return            = 6;
inline constexpr int __ossl_ctrl_extra_chain_cert            = 14;
inline constexpr int __ossl_ctrl_mode                        = 33;
inline constexpr int __ossl_ctrl_set_tlsext_hostname         = 55;
inline constexpr int __ossl_ctrl_chain_cert                  = 89;
inline constexpr int __ossl_ctrl_set_min_proto_version       = 123;
inline constexpr int __ossl_ctrl_set_max_proto_version       = 124;
inline constexpr long __ossl_mode_enable_partial_write       = 0x00000001L;
inline constexpr long __ossl_mode_accept_moving_write_buffer = 0x00000002L;
inline constexpr int __ossl_tlsext_nametype_host_name        = 0;
inline constexpr int __ossl_tls1_2_version                   = 0x0303;
inline constexpr int __ossl_tls1_3_version                   = 0x0304;
inline constexpr int __ossl_ssl_received_shutdown            = 2;
inline constexpr int __ossl_bio_ctrl_pending                 = 10;
inline constexpr int __ossl_tlsext_err_ok                    = 0;
inline constexpr int __ossl_tlsext_err_alert_fatal           = 2;
inline constexpr int __ossl_npn_negotiated                   = 1;
inline constexpr long __ossl_x509_v_ok                       = 0;

using __ossl_alpn_select_cb_t _LIBCPP_NODEBUG =
    int (*)(__ossl_ssl*, const unsigned char**, unsigned char*, const unsigned char*, unsigned int, void*);

// The entry points; every member is the function of the same name in libssl or libcrypto.
struct __ossl_api {
  const __ossl_method* (*__TLS_method)();
  __ossl_ctx* (*__SSL_CTX_new)(const __ossl_method*);
  void (*__SSL_CTX_free)(__ossl_ctx*);
  long (*__SSL_CTX_ctrl)(__ossl_ctx*, int, long, void*);
  void (*__SSL_CTX_set_verify)(__ossl_ctx*, int, void*);
  void (*__SSL_CTX_set_verify_depth)(__ossl_ctx*, int);
  int (*__SSL_CTX_use_certificate)(__ossl_ctx*, __ossl_x509*);
  int (*__SSL_CTX_use_PrivateKey)(__ossl_ctx*, __ossl_pkey*);
  int (*__SSL_CTX_check_private_key)(const __ossl_ctx*);
  int (*__SSL_CTX_set_cipher_list)(__ossl_ctx*, const char*);
  int (*__SSL_CTX_set_ciphersuites)(__ossl_ctx*, const char*);
  int (*__SSL_CTX_set_default_verify_paths)(__ossl_ctx*);
  int (*__SSL_CTX_load_verify_locations)(__ossl_ctx*, const char*, const char*);
  __ossl_store* (*__SSL_CTX_get_cert_store)(const __ossl_ctx*);
  void (*__SSL_CTX_set_alpn_select_cb)(__ossl_ctx*, __ossl_alpn_select_cb_t, void*);
  int (*__SSL_select_next_proto)(
      unsigned char**, unsigned char*, const unsigned char*, unsigned int, const unsigned char*, unsigned int);
  __ossl_ssl* (*__SSL_new)(__ossl_ctx*);
  void (*__SSL_free)(__ossl_ssl*);
  void (*__SSL_set_bio)(__ossl_ssl*, __ossl_bio*, __ossl_bio*);
  void (*__SSL_set_connect_state)(__ossl_ssl*);
  void (*__SSL_set_accept_state)(__ossl_ssl*);
  int (*__SSL_do_handshake)(__ossl_ssl*);
  int (*__SSL_read)(__ossl_ssl*, void*, int);
  int (*__SSL_write)(__ossl_ssl*, const void*, int);
  int (*__SSL_shutdown)(__ossl_ssl*);
  int (*__SSL_get_shutdown)(const __ossl_ssl*);
  int (*__SSL_get_error)(const __ossl_ssl*, int);
  long (*__SSL_ctrl)(__ossl_ssl*, int, long, void*);
  int (*__SSL_set_alpn_protos)(__ossl_ssl*, const unsigned char*, unsigned int);
  void (*__SSL_get0_alpn_selected)(const __ossl_ssl*, const unsigned char**, unsigned int*);
  long (*__SSL_get_verify_result)(const __ossl_ssl*);
  __ossl_vparam* (*__SSL_get0_param)(__ossl_ssl*);
  int (*__X509_VERIFY_PARAM_set1_host)(__ossl_vparam*, const char*, unsigned long);
  int (*__X509_VERIFY_PARAM_set1_ip_asc)(__ossl_vparam*, const char*);
  const char* (*__X509_verify_cert_error_string)(long);
  const __ossl_bio_method* (*__BIO_s_mem)();
  __ossl_bio* (*__BIO_new)(const __ossl_bio_method*);
  __ossl_bio* (*__BIO_new_mem_buf)(const void*, int);
  int (*__BIO_free)(__ossl_bio*);
  int (*__BIO_read)(__ossl_bio*, void*, int);
  int (*__BIO_write)(__ossl_bio*, const void*, int);
  long (*__BIO_ctrl)(__ossl_bio*, int, long, void*);
  __ossl_x509* (*__PEM_read_bio_X509)(__ossl_bio*, __ossl_x509**, void*, void*);
  __ossl_pkey* (*__PEM_read_bio_PrivateKey)(__ossl_bio*, __ossl_pkey**, void*, void*);
  int (*__X509_STORE_add_cert)(__ossl_store*, __ossl_x509*);
  void (*__X509_free)(__ossl_x509*);
  void (*__EVP_PKEY_free)(__ossl_pkey*);
  unsigned long (*__ERR_get_error)();
  void (*__ERR_error_string_n)(unsigned long, char*, size_t);
  void (*__ERR_clear_error)();
};

// Test hook: while set, the loader behaves as if libssl could not be loaded.
_LIBCPP_HIDE_FROM_ABI inline atomic<bool>& __ossl_forced_unavailable() noexcept {
  static atomic<bool> __flag(false);
  return __flag;
}

template <class _Fn>
_LIBCPP_HIDE_FROM_ABI bool __ossl_resolve(void* __ssl, void* __crypto, const char* __name, _Fn*& __fn) noexcept {
  void* __p = __ssl ? ::dlsym(__ssl, __name) : nullptr;
  if (!__p && __crypto)
    __p = ::dlsym(__crypto, __name);
  __fn = reinterpret_cast<_Fn*>(__p);
  return __p != nullptr;
}

// Opens the first of the candidates that loads. The candidates are tried in order.
template <size_t _Np>
_LIBCPP_HIDE_FROM_ABI void* __ossl_open(const char* const (&__names)[_Np]) noexcept {
  for (const char* __n : __names)
    if (void* __h = ::dlopen(__n, RTLD_NOW | RTLD_LOCAL))
      return __h;
  return nullptr;
}

// Where the libraries are looked for. Linux: the dynamic loader's search path (libssl.so.3 first, the unversioned name
// of a development package second). macOS: the system's libssl.dylib is LibreSSL (and on recent releases a stub that
// aborts the process when it is loaded), so the bare name is never tried; OpenSSL 3 is looked for by its versioned name
// through the loader's search path, then in the Homebrew prefixes of Apple silicon and Intel (the keg of openssl@3 and
// the linked lib directory) and MacPorts.
#  ifdef __APPLE__
inline constexpr const char* __ossl_ssl_names[] = {
    "libssl.3.dylib",
    "/opt/homebrew/opt/openssl@3/lib/libssl.3.dylib",
    "/opt/homebrew/lib/libssl.3.dylib",
    "/usr/local/opt/openssl@3/lib/libssl.3.dylib",
    "/usr/local/lib/libssl.3.dylib",
    "/opt/local/lib/libssl.3.dylib"};
inline constexpr const char* __ossl_crypto_names[] = {
    "libcrypto.3.dylib",
    "/opt/homebrew/opt/openssl@3/lib/libcrypto.3.dylib",
    "/opt/homebrew/lib/libcrypto.3.dylib",
    "/usr/local/opt/openssl@3/lib/libcrypto.3.dylib",
    "/usr/local/lib/libcrypto.3.dylib",
    "/opt/local/lib/libcrypto.3.dylib"};
#  elif defined(__FreeBSD__)
inline constexpr const char* __ossl_ssl_names[] = {
    "libssl.so.30", "libssl.so.3", "libssl.so", "/usr/local/lib/libssl.so.3"};
inline constexpr const char* __ossl_crypto_names[] = {
    "libcrypto.so.30", "libcrypto.so.3", "libcrypto.so", "/usr/local/lib/libcrypto.so.3"};
#  else
inline constexpr const char* __ossl_ssl_names[]    = {"libssl.so.3", "libssl.so"};
inline constexpr const char* __ossl_crypto_names[] = {"libcrypto.so.3", "libcrypto.so"};
#  endif

_LIBCPP_HIDE_FROM_ABI inline const __ossl_api* __ossl_load() noexcept {
  static __ossl_api __api;
  void* __ssl = io::__ossl_open(__ossl_ssl_names);
  if (!__ssl)
    return nullptr;
  // libssl depends on libcrypto, so its handle resolves the libcrypto symbols as well; the explicit libcrypto handle is
  // consulted when the dependency is not visible through the handle.
  void* __crypto = io::__ossl_open(__ossl_crypto_names);
#  ifndef __linux__
  // Refuse anything that is not OpenSSL 3 (LibreSSL reports 0x20000000 or has no such function; OpenSSL 1.1 reports
  // 0x1010...): the engine uses the OpenSSL 3 entry points and their semantics. Not checked on Linux, where the
  // library names above already select OpenSSL 3 (a missing entry point fails the load below).
  unsigned long (*__version_num)() = nullptr;
  if (!io::__ossl_resolve(__ssl, __crypto, "OpenSSL_version_num", __version_num) || __version_num() < 0x30000000UL)
    return nullptr;
#  endif
  bool __ok = true;
  auto __r  = [&](const char* __n, auto*& __f) { __ok = io::__ossl_resolve(__ssl, __crypto, __n, __f) && __ok; };
  __r("TLS_method", __api.__TLS_method);
  __r("SSL_CTX_new", __api.__SSL_CTX_new);
  __r("SSL_CTX_free", __api.__SSL_CTX_free);
  __r("SSL_CTX_ctrl", __api.__SSL_CTX_ctrl);
  __r("SSL_CTX_set_verify", __api.__SSL_CTX_set_verify);
  __r("SSL_CTX_set_verify_depth", __api.__SSL_CTX_set_verify_depth);
  __r("SSL_CTX_use_certificate", __api.__SSL_CTX_use_certificate);
  __r("SSL_CTX_use_PrivateKey", __api.__SSL_CTX_use_PrivateKey);
  __r("SSL_CTX_check_private_key", __api.__SSL_CTX_check_private_key);
  __r("SSL_CTX_set_cipher_list", __api.__SSL_CTX_set_cipher_list);
  __r("SSL_CTX_set_ciphersuites", __api.__SSL_CTX_set_ciphersuites);
  __r("SSL_CTX_set_default_verify_paths", __api.__SSL_CTX_set_default_verify_paths);
  __r("SSL_CTX_load_verify_locations", __api.__SSL_CTX_load_verify_locations);
  __r("SSL_CTX_get_cert_store", __api.__SSL_CTX_get_cert_store);
  __r("SSL_CTX_set_alpn_select_cb", __api.__SSL_CTX_set_alpn_select_cb);
  __r("SSL_select_next_proto", __api.__SSL_select_next_proto);
  __r("SSL_new", __api.__SSL_new);
  __r("SSL_free", __api.__SSL_free);
  __r("SSL_set_bio", __api.__SSL_set_bio);
  __r("SSL_set_connect_state", __api.__SSL_set_connect_state);
  __r("SSL_set_accept_state", __api.__SSL_set_accept_state);
  __r("SSL_do_handshake", __api.__SSL_do_handshake);
  __r("SSL_read", __api.__SSL_read);
  __r("SSL_write", __api.__SSL_write);
  __r("SSL_shutdown", __api.__SSL_shutdown);
  __r("SSL_get_shutdown", __api.__SSL_get_shutdown);
  __r("SSL_get_error", __api.__SSL_get_error);
  __r("SSL_ctrl", __api.__SSL_ctrl);
  __r("SSL_set_alpn_protos", __api.__SSL_set_alpn_protos);
  __r("SSL_get0_alpn_selected", __api.__SSL_get0_alpn_selected);
  __r("SSL_get_verify_result", __api.__SSL_get_verify_result);
  __r("SSL_get0_param", __api.__SSL_get0_param);
  __r("X509_VERIFY_PARAM_set1_host", __api.__X509_VERIFY_PARAM_set1_host);
  __r("X509_VERIFY_PARAM_set1_ip_asc", __api.__X509_VERIFY_PARAM_set1_ip_asc);
  __r("X509_verify_cert_error_string", __api.__X509_verify_cert_error_string);
  __r("BIO_s_mem", __api.__BIO_s_mem);
  __r("BIO_new", __api.__BIO_new);
  __r("BIO_new_mem_buf", __api.__BIO_new_mem_buf);
  __r("BIO_free", __api.__BIO_free);
  __r("BIO_read", __api.__BIO_read);
  __r("BIO_write", __api.__BIO_write);
  __r("BIO_ctrl", __api.__BIO_ctrl);
  __r("PEM_read_bio_X509", __api.__PEM_read_bio_X509);
  __r("PEM_read_bio_PrivateKey", __api.__PEM_read_bio_PrivateKey);
  __r("X509_STORE_add_cert", __api.__X509_STORE_add_cert);
  __r("X509_free", __api.__X509_free);
  __r("EVP_PKEY_free", __api.__EVP_PKEY_free);
  __r("ERR_get_error", __api.__ERR_get_error);
  __r("ERR_error_string_n", __api.__ERR_error_string_n);
  __r("ERR_clear_error", __api.__ERR_clear_error);
  return __ok ? &__api : nullptr;
}

// The resolved entry points, loaded once and thread-safely; nullptr if OpenSSL is not available (or while the test hook
// is set).
_LIBCPP_HIDE_FROM_ABI inline const __ossl_api* __ossl_get() noexcept {
  if (io::__ossl_forced_unavailable().load(memory_order_relaxed))
    return nullptr;
  static const __ossl_api* const __p = io::__ossl_load();
  return __p;
}

} // namespace io

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_HAS_IO_TLS

#endif // _LIBCPP___NET_TLS_SUPPORT_OPENSSL_DL_H
