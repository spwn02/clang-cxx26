// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

export namespace std {
#if _LIBCPP_HAS_IO_FILES
  // Stage Two of the networking series (platform I/O), experimental: C++29 with -fexperimental-library
  namespace io {
    using std::io::file_base;
    using std::io::open_flags;
    using std::io::random_access_file;
    using std::io::stream_file;
  } // namespace io
#endif
} // namespace std
