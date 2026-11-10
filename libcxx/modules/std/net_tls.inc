// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

export namespace std {
#if _LIBCPP_HAS_IO_TLS
  // Paper 14 of the networking series (TLS), experimental: C++29 with -fexperimental-library
  namespace io {
    using std::io::make_error_code;
    using std::io::tls_category;
    using std::io::tls_context;
    using std::io::tls_engine_available;
    using std::io::tls_error;
    using std::io::tls_role;
    using std::io::tls_stream;
    using std::io::tls_verify_mode;
    using std::io::tls_version;
  } // namespace io
#endif
} // namespace std
