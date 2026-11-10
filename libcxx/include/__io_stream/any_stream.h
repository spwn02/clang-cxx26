//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#ifndef _LIBCPP___IO_STREAM_ANY_STREAM_H
#define _LIBCPP___IO_STREAM_ANY_STREAM_H

#include <__assert>
#include <__config>
#include <__cstddef/size_t.h>
#include <__io_buffer/buffer.h>
#include <__io_buffer/buffer_algorithms.h>
#include <__io_buffer/buffer_sequence.h>
#include <__io_stream/any_read_stream.h>
#include <__io_stream/any_write_stream.h>
#include <__io_stream/erased.h>
#include <__io_stream/error.h>
#include <__io_stream/stream_concepts.h>
#include <__utility/declval.h>
#include <span>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_EXPERIMENTAL_NETWORKING && _LIBCPP_HAS_THREADS

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

// P4100R1 [8.6]: a non-owning, type-erased Stream: an any_read_stream and an any_write_stream over the same object,
// each with its own operation storage, so one read and one write may be in flight at the same time.
class any_stream : public any_read_stream, public any_write_stream {
public:
  _LIBCPP_HIDE_FROM_ABI any_stream() noexcept = default;
  template <Stream _Obj>
    requires requires(_Obj& __o, span<const mutable_buffer> __r, span<const const_buffer> __w) {
      __o.read_some(__r);
      __o.write_some(__w);
    }
  _LIBCPP_HIDE_FROM_ABI explicit any_stream(_Obj* __o) : any_read_stream(__o), any_write_stream(__o) {}
  any_stream(const any_stream&)                                      = delete;
  any_stream& operator=(const any_stream&)                           = delete;
  _LIBCPP_HIDE_FROM_ABI any_stream(any_stream&&) noexcept            = default;
  _LIBCPP_HIDE_FROM_ABI any_stream& operator=(any_stream&&) noexcept = default;

  _LIBCPP_HIDE_FROM_ABI bool has_value() const noexcept {
    return any_read_stream::has_value() && any_write_stream::has_value();
  }
  _LIBCPP_HIDE_FROM_ABI explicit operator bool() const noexcept { return has_value(); }
};

} // namespace io

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_HAS_EXPERIMENTAL_NETWORKING && _LIBCPP_HAS_THREADS

#endif // _LIBCPP___IO_STREAM_ANY_STREAM_H
