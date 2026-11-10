//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___IO_BUFFER_BUFFER_SEQUENCE_H
#define _LIBCPP___IO_BUFFER_BUFFER_SEQUENCE_H

#include <__concepts/convertible_to.h>
#include <__config>
#include <__io_buffer/buffer.h>
#include <__iterator/concepts.h>
#include <__memory/addressof.h>
#include <__ranges/access.h>
#include <__ranges/concepts.h>
#include <__type_traits/conditional.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_EXPERIMENTAL_NETWORKING

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

// P4100R1 [8.4], B1004 [5]: the element requirement of a buffer sequence. The conversion is from a const lvalue,
// because that is how every algorithm reads an element; a conversion operator must therefore be const. Algorithms
// that read sequences are noexcept, so a conversion that throws terminates the program.
template <class _Tp>
concept ConstBuffer = convertible_to<const _Tp&, const_buffer>;

// A type that converts only to mutable_buffer does not qualify: reaching const_buffer would take two user-defined
// conversions, so it must convert to both.
template <class _Tp>
concept MutableBuffer = ConstBuffer<_Tp> && convertible_to<const _Tp&, mutable_buffer>;

// A buffer sequence is a single buffer or a bidirectional range of them. A single buffer is never treated as a range,
// even if it is one: only its own conversion decides (this is the one place where the concepts differ from B1004's
// is_convertible_v<T, const_buffer> || range formulation, which tests the conversion from T instead of const T&).
template <class _Tp>
concept ConstBufferSequence =
    ConstBuffer<_Tp> || (ranges::bidirectional_range<_Tp> && ConstBuffer<ranges::range_value_t<_Tp>>);

template <class _Tp>
concept MutableBufferSequence =
    ConstBufferSequence<_Tp> &&
    (MutableBuffer<_Tp> ||
     (!ConstBuffer<_Tp> && ranges::bidirectional_range<_Tp> && MutableBuffer<ranges::range_value_t<_Tp>>));

// buffer_sequence_begin / buffer_sequence_end (N4771 [buffer.seq.access]), as customization point objects: a single
// buffer is a sequence of one element (the iterator is a pointer to the buffer itself), every other sequence is
// asked through std::ranges::begin and std::ranges::end.
namespace __buffer_sequence_begin {
struct __fn {
  template <ConstBuffer _Buf>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr const _Buf* operator()(const _Buf& __b) const noexcept {
    return std::addressof(__b);
  }

  template <ConstBufferSequence _Seq>
    requires(!ConstBuffer<_Seq>)
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Seq& __s) const noexcept {
    return ranges::begin(__s);
  }

  template <ConstBufferSequence _Seq>
    requires(!ConstBuffer<_Seq>)
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(const _Seq& __s) const noexcept {
    return ranges::begin(__s);
  }
};
} // namespace __buffer_sequence_begin

inline namespace __cpo {
inline constexpr auto buffer_sequence_begin = __buffer_sequence_begin::__fn{};
} // namespace __cpo

namespace __buffer_sequence_end {
struct __fn {
  template <ConstBuffer _Buf>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr const _Buf* operator()(const _Buf& __b) const noexcept {
    return std::addressof(__b) + 1;
  }

  template <ConstBufferSequence _Seq>
    requires(!ConstBuffer<_Seq>)
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Seq& __s) const noexcept {
    return ranges::end(__s);
  }

  template <ConstBufferSequence _Seq>
    requires(!ConstBuffer<_Seq>)
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(const _Seq& __s) const noexcept {
    return ranges::end(__s);
  }
};
} // namespace __buffer_sequence_end

inline namespace __cpo {
inline constexpr auto buffer_sequence_end = __buffer_sequence_end::__fn{};
} // namespace __cpo

// B1004 [6]: mutable_buffer for a mutable buffer sequence, const_buffer for any other.
template <class _Seq>
using buffer_type = conditional_t<MutableBufferSequence<_Seq>, mutable_buffer, const_buffer>;

} // namespace io

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_HAS_EXPERIMENTAL_NETWORKING

#endif // _LIBCPP___IO_BUFFER_BUFFER_SEQUENCE_H
