// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

export namespace std {
#if _LIBCPP_HAS_EXPERIMENTAL_NETWORKING
  // Papers 4 and 5 of P4100R1, experimental: C++29 with -fexperimental-library
  namespace io {
    using std::io::const_buffer;
    using std::io::mutable_buffer;
    using std::io::operator+;

    using std::io::const_buffer_pair;
    using std::io::mutable_buffer_pair;

    using std::io::ConstBuffer;
    using std::io::ConstBufferSequence;
    using std::io::MutableBuffer;
    using std::io::MutableBufferSequence;

    using std::io::buffer_type;

    using std::io::buffer_copy;
    using std::io::buffer_empty;
    using std::io::buffer_length;
    using std::io::buffer_sequence_begin;
    using std::io::buffer_sequence_end;
    using std::io::buffer_size;

    using std::io::size_tag;
    using std::io::slice_how;
    using std::io::slice_tag;
    using std::io::tag_invoke;

    using std::io::keep_prefix;
    using std::io::keep_suffix;
    using std::io::prefix;
    using std::io::remove_prefix;
    using std::io::remove_suffix;
    using std::io::sans_prefix;
    using std::io::sans_suffix;
    using std::io::suffix;

    using std::io::make_buffer;

    using std::io::DynamicBuffer;

    using std::io::circular_dynamic_buffer;
    using std::io::flat_dynamic_buffer;
    using std::io::string_dynamic_buffer;
    using std::io::vector_dynamic_buffer;
  } // namespace io
#endif
} // namespace std
