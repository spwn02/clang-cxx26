//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___IO_BUFFER_BUFFER_PAIR_H
#define _LIBCPP___IO_BUFFER_BUFFER_PAIR_H

#include <__config>
#include <__cstddef/size_t.h>
#include <__io_buffer/buffer.h>
#include <__io_buffer/buffer_slice.h>
#include <array>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_EXPERIMENTAL_NETWORKING

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

// B1004 [7]: two buffers, the shape of a region that wraps around the end of a ring.
using const_buffer_pair   = array<const_buffer, 2>;
using mutable_buffer_pair = array<mutable_buffer, 2>;

namespace __pair_detail {
template <class _Buf>
_LIBCPP_HIDE_FROM_ABI constexpr void __slice(array<_Buf, 2>& __p, slice_how __how, size_t __n) noexcept {
  if (__how == slice_how::keep_prefix) {
    // Keep the first n bytes: the first buffer keeps min(n, size), the second what is left of n.
    size_t __first = __p[0].size() < __n ? __p[0].size() : __n;
    __p[0]         = _Buf(__p[0].data(), __first);
    __p[1]         = _Buf(__p[1].data(), __n - __first < __p[1].size() ? __n - __first : __p[1].size());
  } else {
    // Remove the first n bytes across the buffer boundary.
    size_t __first = __p[0].size() < __n ? __p[0].size() : __n;
    __p[0] += __first;
    __p[1] += __n - __first;
  }
}
} // namespace __pair_detail

_LIBCPP_HIDE_FROM_ABI constexpr void
tag_invoke(slice_tag, const_buffer_pair& __p, slice_how __how, size_t __n) noexcept {
  __pair_detail::__slice(__p, __how, __n);
}

_LIBCPP_HIDE_FROM_ABI constexpr void
tag_invoke(slice_tag, mutable_buffer_pair& __p, slice_how __how, size_t __n) noexcept {
  __pair_detail::__slice(__p, __how, __n);
}

} // namespace io

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_HAS_EXPERIMENTAL_NETWORKING

#endif // _LIBCPP___IO_BUFFER_BUFFER_PAIR_H
