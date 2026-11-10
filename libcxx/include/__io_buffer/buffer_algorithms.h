//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___IO_BUFFER_BUFFER_ALGORITHMS_H
#define _LIBCPP___IO_BUFFER_BUFFER_ALGORITHMS_H

#include <__concepts/same_as.h>
#include <__config>
#include <__cstddef/size_t.h>
#include <__io_buffer/buffer.h>
#include <__io_buffer/buffer_sequence.h>
#include <__iterator/distance.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_EXPERIMENTAL_NETWORKING

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

// B1004 [7]: the customization tag of buffer_size. A user-defined sequence can compute its size in O(1) by providing
// a function found by argument-dependent lookup,
//   size_t tag_invoke(size_tag, const my_sequence&) noexcept;
// (the tag_invoke protocol of P1895, which was removed from the C++23 working draft; see the deviation notes).
struct size_tag {};

namespace __tag_invoke_poison {
void tag_invoke() = delete;

template <class _Seq>
concept __has_size_tag_invoke = requires(const _Seq& __s) {
  { tag_invoke(size_tag{}, __s) } noexcept -> same_as<size_t>; // NOLINT(libcpp-robust-against-adl)
};
} // namespace __tag_invoke_poison

namespace __buffer_size {
struct __fn {
  // The total number of bytes of all buffers of the sequence; ranges::size would count the buffers instead.
  template <ConstBufferSequence _Seq>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr size_t operator()(const _Seq& __s) const noexcept {
    if constexpr (__tag_invoke_poison::__has_size_tag_invoke<_Seq>) {
      return tag_invoke(size_tag{}, __s); // NOLINT(libcpp-robust-against-adl)
    } else {
      size_t __total = 0;
      auto __first   = buffer_sequence_begin(__s);
      auto __last    = buffer_sequence_end(__s);
      for (; __first != __last; ++__first)
        __total += const_buffer(*__first).size();
      return __total;
    }
  }
};
} // namespace __buffer_size

inline namespace __cpo {
inline constexpr auto buffer_size = __buffer_size::__fn{};
} // namespace __cpo

namespace __buffer_empty {
struct __fn {
  // True if every buffer of the sequence has size zero (or the sequence has no buffers). Stops at the first buffer
  // that has data, which makes it cheaper than buffer_size(s) == 0.
  template <ConstBufferSequence _Seq>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr bool operator()(const _Seq& __s) const noexcept {
    auto __first = buffer_sequence_begin(__s);
    auto __last  = buffer_sequence_end(__s);
    for (; __first != __last; ++__first)
      if (const_buffer(*__first).size() != 0)
        return false;
    return true;
  }
};
} // namespace __buffer_empty

inline namespace __cpo {
inline constexpr auto buffer_empty = __buffer_empty::__fn{};
} // namespace __cpo

namespace __buffer_length {
struct __fn {
  // The number of buffers of the sequence (1 for a single buffer), not the number of bytes.
  template <ConstBufferSequence _Seq>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr size_t operator()(const _Seq& __s) const noexcept {
    return static_cast<size_t>(std::distance(buffer_sequence_begin(__s), buffer_sequence_end(__s)));
  }
};
} // namespace __buffer_length

inline namespace __cpo {
inline constexpr auto buffer_length = __buffer_length::__fn{};
} // namespace __cpo

namespace __buffer_copy {
struct __fn {
  // N4771 [buffer.copy]: copies min(buffer_size(dest), buffer_size(source), at_most) bytes, filling each destination
  // buffer completely before the next one, as if by memcpy per contiguous chunk. The regions must not overlap.
  template <MutableBufferSequence _Dest, ConstBufferSequence _Src>
  _LIBCPP_HIDE_FROM_ABI size_t
  operator()(const _Dest& __dest, const _Src& __src, size_t __at_most = size_t(-1)) const noexcept {
    size_t __total = 0;
    size_t __off_s = 0;
    size_t __off_d = 0;
    auto __is      = buffer_sequence_begin(__src);
    auto __es      = buffer_sequence_end(__src);
    auto __id      = buffer_sequence_begin(__dest);
    auto __ed      = buffer_sequence_end(__dest);
    while (__total < __at_most && __is != __es && __id != __ed) {
      const_buffer __bs   = *__is;
      mutable_buffer __bd = *__id;
      __bs += __off_s;
      __bd += __off_d;
      size_t __n = __bs.size() < __bd.size() ? __bs.size() : __bd.size();
      if (__n > __at_most - __total)
        __n = __at_most - __total;
      if (__n != 0)
        __builtin_memcpy(__bd.data(), __bs.data(), __n);
      __total += __n;
      if (__n == __bd.size()) {
        ++__id;
        __off_d = 0;
      } else {
        __off_d += __n;
      }
      if (__n == __bs.size()) {
        ++__is;
        __off_s = 0;
      } else {
        __off_s += __n;
      }
    }
    return __total;
  }
};
} // namespace __buffer_copy

inline namespace __cpo {
inline constexpr auto buffer_copy = __buffer_copy::__fn{};
} // namespace __cpo

} // namespace io

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_HAS_EXPERIMENTAL_NETWORKING

#endif // _LIBCPP___IO_BUFFER_BUFFER_ALGORITHMS_H
