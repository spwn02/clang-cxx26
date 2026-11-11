//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___IO_BUFFER_MAKE_BUFFER_H
#define _LIBCPP___IO_BUFFER_MAKE_BUFFER_H

#include <__concepts/convertible_to.h>
#include <__config>
#include <__cstddef/size_t.h>
#include <__io_buffer/buffer.h>
#include <__ranges/access.h>
#include <__ranges/concepts.h>
#include <__ranges/data.h>
#include <__ranges/size.h>
#include <__type_traits/is_const.h>
#include <__type_traits/is_trivially_copyable.h>
#include <__type_traits/remove_reference.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_EXPERIMENTAL_NETWORKING

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

// The counterpart of N4771's buffer(...) creation functions (named make_buffer, as in Capy, because buffer is too
// good a name for a function of namespace std::io). The result refers to the storage of the argument, which must
// outlive it; its size is the size in bytes, optionally clamped to max_size.
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr mutable_buffer make_buffer(void* __p, size_t __n) noexcept {
  return mutable_buffer(__p, __n);
}

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr mutable_buffer make_buffer(void* __p, size_t __n, size_t __max) noexcept {
  return mutable_buffer(__p, __n < __max ? __n : __max);
}

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr const_buffer make_buffer(const void* __p, size_t __n) noexcept {
  return const_buffer(__p, __n);
}

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr const_buffer
make_buffer(const void* __p, size_t __n, size_t __max) noexcept {
  return const_buffer(__p, __n < __max ? __n : __max);
}

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr mutable_buffer make_buffer(const mutable_buffer& __b) noexcept {
  return __b;
}

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr mutable_buffer
make_buffer(const mutable_buffer& __b, size_t __max) noexcept {
  return mutable_buffer(__b.data(), __b.size() < __max ? __b.size() : __max);
}

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr const_buffer make_buffer(const const_buffer& __b) noexcept { return __b; }

[[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr const_buffer make_buffer(const const_buffer& __b, size_t __max) noexcept {
  return const_buffer(__b.data(), __b.size() < __max ? __b.size() : __max);
}

namespace __make_buffer_detail {
// A range of trivially copyable elements whose storage is contiguous. Things that already are buffers are not
// ranges here: they have their own overloads above.
template <class _Rng>
concept __storage_range =
    ranges::contiguous_range<_Rng> && ranges::sized_range<_Rng> && !convertible_to<_Rng, const_buffer> &&
    !convertible_to<_Rng, mutable_buffer> && is_trivially_copyable_v<ranges::range_value_t<_Rng>>;

template <class _Rng>
concept __mutable_storage_range =
    __storage_range<_Rng> && ranges::borrowed_range<_Rng> &&
    !is_const_v<remove_reference_t<ranges::range_reference_t<_Rng>>>;

template <class _Rng>
concept __const_storage_range = __storage_range<_Rng>;

template <class _Rng>
_LIBCPP_HIDE_FROM_ABI constexpr size_t __bytes(_Rng& __r) noexcept {
  return static_cast<size_t>(ranges::size(__r)) * sizeof(ranges::range_value_t<_Rng>);
}
} // namespace __make_buffer_detail

// A mutable contiguous range: an lvalue, or a borrowed range such as a span (a temporary vector would dangle).
template <__make_buffer_detail::__mutable_storage_range _Rng>
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI mutable_buffer make_buffer(_Rng&& __r) noexcept {
  return mutable_buffer(ranges::size(__r) != 0 ? ranges::data(__r) : nullptr, __make_buffer_detail::__bytes(__r));
}

template <__make_buffer_detail::__mutable_storage_range _Rng>
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI mutable_buffer make_buffer(_Rng&& __r, size_t __max) noexcept {
  size_t __n = __make_buffer_detail::__bytes(__r);
  return mutable_buffer(ranges::size(__r) != 0 ? ranges::data(__r) : nullptr, __n < __max ? __n : __max);
}

// Any contiguous range through a const reference, which includes string literals and string_views.
template <__make_buffer_detail::__const_storage_range _Rng>
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI const_buffer make_buffer(const _Rng& __r) noexcept {
  return const_buffer(ranges::size(__r) != 0 ? ranges::data(__r) : nullptr, __make_buffer_detail::__bytes(__r));
}

template <__make_buffer_detail::__const_storage_range _Rng>
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI const_buffer make_buffer(const _Rng& __r, size_t __max) noexcept {
  size_t __n = __make_buffer_detail::__bytes(__r);
  return const_buffer(ranges::size(__r) != 0 ? ranges::data(__r) : nullptr, __n < __max ? __n : __max);
}

} // namespace io

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_HAS_EXPERIMENTAL_NETWORKING

#endif // _LIBCPP___IO_BUFFER_MAKE_BUFFER_H
