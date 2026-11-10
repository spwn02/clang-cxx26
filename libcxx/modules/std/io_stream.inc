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
  // Paper 6 of P4100R1, experimental: C++29 with -fexperimental-library
  namespace io {
    using std::io::cond;
    using std::io::error;
    using std::io::io_category;
    using std::io::io_condition_category;
    using std::io::io_result;
    using std::io::io_task;
    using std::io::make_error_code;
    using std::io::make_error_condition;

    using std::io::BufferSink;
    using std::io::BufferSource;
    using std::io::ReadSource;
    using std::io::ReadStream;
    using std::io::Stream;
    using std::io::WriteSink;
    using std::io::WriteStream;

    using std::io::any_buffer_sink;
    using std::io::any_buffer_source;
    using std::io::any_read_source;
    using std::io::any_read_stream;
    using std::io::any_stream;
    using std::io::any_write_sink;
    using std::io::any_write_stream;

    using std::io::read;
    using std::io::write;
  } // namespace io
#endif
} // namespace std
