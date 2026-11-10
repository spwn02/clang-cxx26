//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___IO_STREAM_STREAM_CONCEPTS_H
#define _LIBCPP___IO_STREAM_STREAM_CONCEPTS_H

#include <__concepts/convertible_to.h>
#include <__config>
#include <__cstddef/size_t.h>
#include <__io/io_awaitable.h>
#include <__io_buffer/buffer.h>
#include <__io_buffer/buffer_sequence.h>
#include <__io_stream/error.h>
#include <__system_error/error_code.h>
#include <__tuple/tuple_element.h>
#include <__tuple/tuple_size.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/declval.h>
#include <__utility/integer_sequence.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_EXPERIMENTAL_NETWORKING && _LIBCPP_HAS_THREADS

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

// Satisfy the buffer-sequence concepts but cannot be default-constructed; used only as unevaluated function arguments to
// check that an operation accepts any buffer sequence.
struct __const_buffer_archetype {
  __const_buffer_archetype()                                              = delete;
  __const_buffer_archetype(const __const_buffer_archetype&)               = default;
  __const_buffer_archetype& operator=(const __const_buffer_archetype&)    = default;
  _LIBCPP_HIDE_FROM_ABI operator const_buffer() const noexcept { return {}; }
};

struct __mutable_buffer_archetype {
  __mutable_buffer_archetype()                                              = delete;
  __mutable_buffer_archetype(const __mutable_buffer_archetype&)             = default;
  __mutable_buffer_archetype& operator=(const __mutable_buffer_archetype&)  = default;
  _LIBCPP_HIDE_FROM_ABI operator const_buffer() const noexcept { return {}; }
  _LIBCPP_HIDE_FROM_ABI operator mutable_buffer() const noexcept { return {}; }
};

// The result of awaiting _Aw is tuple-like with exactly the elements _Ts (each convertible).
template <class _Result, class _Seq, class... _Ts>
inline constexpr bool __decomposes_to_impl = false;

template <class _Result, size_t... _Is, class... _Ts>
inline constexpr bool __decomposes_to_impl<_Result, index_sequence<_Is...>, _Ts...> =
    (convertible_to<tuple_element_t<_Is, _Result>, _Ts> && ...);

template <class _Aw, class... _Ts>
concept __awaitable_decomposes_to =
    IoAwaitable<remove_cvref_t<_Aw>> &&
    requires(remove_cvref_t<_Aw>& __a) {
      { __a.await_ready() } -> convertible_to<bool>;
      __a.await_resume();
    } &&
    requires {
      typename tuple_size<remove_cvref_t<decltype(std::declval<remove_cvref_t<_Aw>&>().await_resume())>>::type;
    } &&
    (tuple_size_v<remove_cvref_t<decltype(std::declval<remove_cvref_t<_Aw>&>().await_resume())>> == sizeof...(_Ts)) &&
    __decomposes_to_impl<remove_cvref_t<decltype(std::declval<remove_cvref_t<_Aw>&>().await_resume())>,
                         make_index_sequence<sizeof...(_Ts)>,
                         _Ts...>;

// P4100R1 [8.6], B1005 [2.1]: read_some(MutableBufferSequence) returns an IoAwaitable of (error_code, size_t).
template <class _Tp>
concept ReadStream = requires(_Tp& __s, __mutable_buffer_archetype __b) {
  { __s.read_some(__b) } -> __awaitable_decomposes_to<error_code, size_t>;
};

template <class _Tp>
concept WriteStream = requires(_Tp& __s, __const_buffer_archetype __b) {
  { __s.write_some(__b) } -> __awaitable_decomposes_to<error_code, size_t>;
};

template <class _Tp>
concept Stream = ReadStream<_Tp> && WriteStream<_Tp>;

// Complete reads: on success n == buffer_size(buffers); end of stream before that is an error (io::error::eof).
template <class _Tp>
concept ReadSource = requires(_Tp& __s, __mutable_buffer_archetype __b) {
  { __s.read(__b) } -> __awaitable_decomposes_to<error_code, size_t>;
};

// Complete writes plus end-of-stream signalling.
template <class _Tp>
concept WriteSink = requires(_Tp& __s, __const_buffer_archetype __b, bool __eof) {
  { __s.write(__b) } -> __awaitable_decomposes_to<error_code, size_t>;
  { __s.write(__b, __eof) } -> __awaitable_decomposes_to<error_code, size_t>;
  { __s.write_eof() } -> __awaitable_decomposes_to<error_code>;
};

// Callee-owned buffers: pull fills an array of descriptors from internal storage (count 0 and no error means the source is
// exhausted), consume advances the read position.
template <class _Tp>
concept BufferSource = requires(_Tp& __s, const_buffer* __arr, size_t __n) {
  { __s.pull(__arr, __n) } -> __awaitable_decomposes_to<error_code, size_t>;
  __s.consume(__n);
};

// prepare is synchronous and returns the number of descriptors filled; commit finalizes bytes written into them.
template <class _Tp>
concept BufferSink = requires(_Tp& __s, mutable_buffer* __arr, size_t __n, bool __eof) {
  { __s.prepare(__arr, __n) } -> convertible_to<size_t>;
  { __s.commit(__n) } -> __awaitable_decomposes_to<error_code>;
  { __s.commit(__n, __eof) } -> __awaitable_decomposes_to<error_code>;
  { __s.commit_eof() } -> __awaitable_decomposes_to<error_code>;
};

} // namespace io

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_HAS_EXPERIMENTAL_NETWORKING && _LIBCPP_HAS_THREADS

#endif // _LIBCPP___IO_STREAM_STREAM_CONCEPTS_H
