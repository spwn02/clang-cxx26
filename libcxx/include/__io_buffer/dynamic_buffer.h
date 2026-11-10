//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___IO_BUFFER_DYNAMIC_BUFFER_H
#define _LIBCPP___IO_BUFFER_DYNAMIC_BUFFER_H

#include <__concepts/constructible.h>
#include <__concepts/same_as.h>
#include <__config>
#include <__cstddef/size_t.h>
#include <__io_buffer/buffer_sequence.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_EXPERIMENTAL_NETWORKING

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

// P4100R1 [8.5], N4771 [buffer.reqmts.dynamicbuffer]: a growable byte buffer whose storage is split into readable
// bytes followed by writable bytes. prepare(n) returns n writable bytes (and throws length_error if size() + n exceeds
// max_size()), commit(n) turns the first n of them into readable bytes, consume(n) discards n readable bytes from the
// front. Any sequence obtained from data() or prepare() is invalidated by the next call to prepare, commit or consume.
template <class _Tp>
concept DynamicBuffer = move_constructible<_Tp> && requires(_Tp __x, const _Tp __cx, size_t __n) {
  typename _Tp::const_buffers_type;
  typename _Tp::mutable_buffers_type;
  requires ConstBufferSequence<typename _Tp::const_buffers_type>;
  requires MutableBufferSequence<typename _Tp::mutable_buffers_type>;
  { __cx.size() } -> same_as<size_t>;
  { __cx.max_size() } -> same_as<size_t>;
  { __cx.capacity() } -> same_as<size_t>;
  { __cx.data() } -> same_as<typename _Tp::const_buffers_type>;
  { __x.prepare(__n) } -> same_as<typename _Tp::mutable_buffers_type>;
  __x.commit(__n);
  __x.consume(__n);
};

} // namespace io

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_HAS_EXPERIMENTAL_NETWORKING

#endif // _LIBCPP___IO_BUFFER_DYNAMIC_BUFFER_H
