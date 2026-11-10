// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

export namespace std {
#if _LIBCPP_HAS_EXPERIMENTAL_NETWORKING && _LIBCPP_HAS_THREADS
  // Paper 8 of the networking series (P4092R1, P4093R1), experimental: C++29 with -fexperimental-library
  namespace io {
    using std::io::await_sender;
    using std::io::sender_awaitable;

    using std::io::awaitable_sender;
    using std::io::as_sender;
    using std::io::get_io_executor;
    using std::io::get_io_executor_t;
    using std::io::io_sender_env;

    using std::io::split_ec;
    using std::io::split_ec_t;
  } // namespace io
#endif
} // namespace std
