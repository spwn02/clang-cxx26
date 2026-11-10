//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___IO_BUFFER_BUFFER_SLICE_H
#define _LIBCPP___IO_BUFFER_BUFFER_SLICE_H

#include <__config>
#include <__cstddef/size_t.h>
#include <__io_buffer/buffer.h>
#include <__io_buffer/buffer_algorithms.h>
#include <__io_buffer/buffer_sequence.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_EXPERIMENTAL_NETWORKING

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

// B1004 [4], [7]: byte-granularity slicing. std::views::drop and friends work on whole elements; slicing a buffer
// sequence has to shrink the first or the last buffer. A sequence type opts in with a single function found by
// argument-dependent lookup,
//   void tag_invoke(slice_tag, my_sequence&, slice_how, size_t) noexcept;
// that handles both slice_how values, so that a type cannot support one operation without the other. There is no
// generic slice_of fallback (see the deviation notes): a sequence that does not provide the function cannot be sliced.
struct slice_tag {};

enum class slice_how {
  remove_prefix, // trim n bytes from the front
  keep_prefix    // keep only the first n bytes
};

_LIBCPP_HIDE_FROM_ABI constexpr void tag_invoke(slice_tag, const_buffer& __b, slice_how __how, size_t __n) noexcept {
  __n = __n < __b.size() ? __n : __b.size();
  if (__how == slice_how::remove_prefix)
    __b += __n;
  else
    __b = const_buffer(__b.data(), __n);
}

_LIBCPP_HIDE_FROM_ABI constexpr void tag_invoke(slice_tag, mutable_buffer& __b, slice_how __how, size_t __n) noexcept {
  __n = __n < __b.size() ? __n : __b.size();
  if (__how == slice_how::remove_prefix)
    __b += __n;
  else
    __b = mutable_buffer(__b.data(), __n);
}

namespace __slice_poison {
void tag_invoke() = delete;

template <class _Seq>
concept __sliceable = ConstBufferSequence<_Seq> && requires(_Seq& __s, slice_how __h, size_t __n) {
  { tag_invoke(slice_tag{}, __s, __h, __n) } noexcept; // NOLINT(libcpp-robust-against-adl)
};
} // namespace __slice_poison

// In-place operations. n is clamped: keeping more than the sequence holds keeps everything, removing more removes
// everything.
template <class _Seq>
  requires __slice_poison::__sliceable<_Seq>
_LIBCPP_HIDE_FROM_ABI constexpr void keep_prefix(_Seq& __s, size_t __n) noexcept {
  tag_invoke(slice_tag{}, __s, slice_how::keep_prefix, __n); // NOLINT(libcpp-robust-against-adl)
}

template <class _Seq>
  requires __slice_poison::__sliceable<_Seq>
_LIBCPP_HIDE_FROM_ABI constexpr void remove_prefix(_Seq& __s, size_t __n) noexcept {
  tag_invoke(slice_tag{}, __s, slice_how::remove_prefix, __n); // NOLINT(libcpp-robust-against-adl)
}

template <class _Seq>
  requires __slice_poison::__sliceable<_Seq>
_LIBCPP_HIDE_FROM_ABI constexpr void keep_suffix(_Seq& __s, size_t __n) noexcept {
  size_t __size = buffer_size(__s);
  if (__n < __size)
    io::remove_prefix(__s, __size - __n);
}

template <class _Seq>
  requires __slice_poison::__sliceable<_Seq>
_LIBCPP_HIDE_FROM_ABI constexpr void remove_suffix(_Seq& __s, size_t __n) noexcept {
  size_t __size = buffer_size(__s);
  io::keep_prefix(__s, __n < __size ? __size - __n : 0);
}

// Non-mutating operations return a sliced copy.
template <class _Seq>
  requires __slice_poison::__sliceable<_Seq>
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr _Seq prefix(_Seq __s, size_t __n) noexcept {
  io::keep_prefix(__s, __n);
  return __s;
}

template <class _Seq>
  requires __slice_poison::__sliceable<_Seq>
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr _Seq suffix(_Seq __s, size_t __n) noexcept {
  io::keep_suffix(__s, __n);
  return __s;
}

template <class _Seq>
  requires __slice_poison::__sliceable<_Seq>
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr _Seq sans_prefix(_Seq __s, size_t __n) noexcept {
  io::remove_prefix(__s, __n);
  return __s;
}

template <class _Seq>
  requires __slice_poison::__sliceable<_Seq>
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr _Seq sans_suffix(_Seq __s, size_t __n) noexcept {
  io::remove_suffix(__s, __n);
  return __s;
}

} // namespace io

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_HAS_EXPERIMENTAL_NETWORKING

#endif // _LIBCPP___IO_BUFFER_BUFFER_SLICE_H
