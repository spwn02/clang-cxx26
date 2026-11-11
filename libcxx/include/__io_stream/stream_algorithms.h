//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___IO_STREAM_STREAM_ALGORITHMS_H
#define _LIBCPP___IO_STREAM_STREAM_ALGORITHMS_H

#include <__config>
#include <__cstddef/size_t.h>
#include <__io/task.h>
#include <__io_buffer/buffer.h>
#include <__io_buffer/buffer_algorithms.h>
#include <__io_buffer/buffer_sequence.h>
#include <__io_buffer/dynamic_buffer.h>
#include <__io_stream/erased.h>
#include <__io_stream/error.h>
#include <__io_stream/stream_concepts.h>
#include <__system_error/errc.h>
#include <__system_error/error_code.h>
#include <__type_traits/is_array.h>
#include <__type_traits/is_trivially_copyable.h>
#include <__utility/move.h>
#include <span>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_EXPERIMENTAL_NETWORKING && _LIBCPP_HAS_THREADS

_LIBCPP_PUSH_MACROS
#  include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

// Capy: io_task<Ts...> is task<io_result<Ts...>>.
template <class... _Ts>
using io_task = task<io_result<_Ts...>>;

// A buffer sequence that is a small, trivially copyable value (a buffer, a pair of buffers, a span): read() and write()
// copy it into the frame of the operation, so a temporary such as io::make_buffer(p, n) is fine even though the task is
// lazy. Any other sequence (a vector of buffers, an array of more than a few buffers, a user type) is held by reference
// and must outlive the operation; a temporary of such a type does not compile.
template <class _Seq>
inline constexpr bool __io_copied_sequence_v =
    !is_array_v<_Seq> && is_trivially_copyable_v<_Seq> && sizeof(_Seq) <= 4 * sizeof(void*);

// _Seq is the type of the sequence, _Held the type that the frame stores: _Seq by value, or const _Seq&.
template <class _Stream, class _Seq, class _Held>
_LIBCPP_HIDE_FROM_ABI io_task<size_t> __read_impl(_Stream& __stream, _Held __buffers) {
  const size_t __total = io::buffer_size(__buffers);
  size_t __done        = 0;
  while (__done < __total) {
    mutable_buffer __rest[__erased_max_buffers];
    const size_t __count = io::__gather_buffers<mutable_buffer>(__buffers, __done, __rest);
    auto [__ec, __n]     = co_await __stream.read_some(span<const mutable_buffer>(__rest, __count));
    __done += __n;
    if (__ec)
      co_return io_result<size_t>{__ec, __done};
  }
  co_return io_result<size_t>{error_code{}, __done};
}

template <class _Stream, class _Seq, class _Held>
_LIBCPP_HIDE_FROM_ABI io_task<size_t> __write_impl(_Stream& __stream, _Held __buffers) {
  const size_t __total = io::buffer_size(__buffers);
  size_t __done        = 0;
  while (__done < __total) {
    const_buffer __rest[__erased_max_buffers];
    const size_t __count = io::__gather_buffers<const_buffer>(__buffers, __done, __rest);
    auto [__ec, __n]     = co_await __stream.write_some(span<const const_buffer>(__rest, __count));
    __done += __n;
    if (__ec)
      co_return io_result<size_t>{__ec, __done};
  }
  co_return io_result<size_t>{error_code{}, __done};
}

// P4100R1 [8.6], B1005 [4.2]: reads until the whole buffer sequence is filled or an error occurs, by calling read_some
// repeatedly. The result carries the bytes transferred so far: on end of stream before the sequence was full the error
// is io::error::eof and n < buffer_size(buffers). A buffer, a pair of buffers or another small trivially copyable
// sequence is copied into the operation. Any other sequence is held by reference: it must outlive the operation (the
// task is lazy), so pass a named sequence or await the call in the same full-expression; a temporary is rejected.
template <ReadStream _Stream, MutableBufferSequence _Seq>
  requires __io_copied_sequence_v<_Seq>
_LIBCPP_HIDE_FROM_ABI io_task<size_t> read(_Stream& __stream, _Seq __buffers) {
  return io::__read_impl<_Stream, _Seq, _Seq>(__stream, std::move(__buffers));
}
template <ReadStream _Stream, MutableBufferSequence _Seq>
  requires(!__io_copied_sequence_v<_Seq>)
_LIBCPP_HIDE_FROM_ABI io_task<size_t> read(_Stream& __stream, const _Seq& __buffers) {
  return io::__read_impl<_Stream, _Seq, const _Seq&>(__stream, __buffers);
}
template <ReadStream _Stream, MutableBufferSequence _Seq>
  requires(!__io_copied_sequence_v<_Seq>)
io_task<size_t> read(_Stream&, const _Seq&&) = delete; // a temporary would dangle in the lazy task

// Writes until the whole buffer sequence is consumed or an error occurs, by calling write_some repeatedly (the same
// rules for the lifetime of the sequence).
template <WriteStream _Stream, ConstBufferSequence _Seq>
  requires __io_copied_sequence_v<_Seq>
_LIBCPP_HIDE_FROM_ABI io_task<size_t> write(_Stream& __stream, _Seq __buffers) {
  return io::__write_impl<_Stream, _Seq, _Seq>(__stream, std::move(__buffers));
}
template <WriteStream _Stream, ConstBufferSequence _Seq>
  requires(!__io_copied_sequence_v<_Seq>)
_LIBCPP_HIDE_FROM_ABI io_task<size_t> write(_Stream& __stream, const _Seq& __buffers) {
  return io::__write_impl<_Stream, _Seq, const _Seq&>(__stream, __buffers);
}
template <WriteStream _Stream, ConstBufferSequence _Seq>
  requires(!__io_copied_sequence_v<_Seq>)
io_task<size_t> write(_Stream&, const _Seq&&) = delete; // a temporary would dangle in the lazy task

// Reads from the stream into a dynamic buffer until end of stream, which is a success here (the goal is to read
// everything). The buffer grows by 1.5x per round from `__initial` bytes. If the buffer is full (size() == max_size())
// the result is errc::no_buffer_space. Returns the bytes appended.
template <ReadStream _Stream, DynamicBuffer _Db>
_LIBCPP_HIDE_FROM_ABI io_task<size_t> read(_Stream& __stream, _Db& __db, size_t __initial = 2048) {
  size_t __total = 0;
  size_t __chunk = __initial == 0 ? 1 : __initial;
  for (;;) {
    const size_t __room = __db.max_size() - __db.size();
    if (__room == 0)
      co_return io_result<size_t>{std::make_error_code(errc::no_buffer_space), __total};
    const size_t __ask = __chunk < __room ? __chunk : __room;
    auto __mb          = __db.prepare(__ask);
    auto [__ec, __n]   = co_await __stream.read_some(__mb);
    __db.commit(__n);
    __total += __n;
    if (__ec == cond::eof)
      co_return io_result<size_t>{error_code{}, __total};
    if (__ec)
      co_return io_result<size_t>{__ec, __total};
    if (__n == __ask)
      __chunk += __chunk / 2 + 1;
  }
}

} // namespace io

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP_HAS_EXPERIMENTAL_NETWORKING && _LIBCPP_HAS_THREADS

#endif // _LIBCPP___IO_STREAM_STREAM_ALGORITHMS_H
