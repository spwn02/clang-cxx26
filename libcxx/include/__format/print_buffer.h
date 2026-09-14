// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___FORMAT_PRINT_BUFFER_H
#define _LIBCPP___FORMAT_PRINT_BUFFER_H

#include <__config>
#include <__iterator/concepts.h>
#include <__memory/addressof.h>
#include <__utility/forward.h>
#include <__utility/move.h>
#include <cstddef>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 23

namespace __format {

// The print path must not allocate a string proportional to the formatted
// output. Keep writes reasonably large while bounding the amount of data held
// between calls to the target stream.
template <class _CharT, class _Flush>
class __print_buffer {
  static constexpr size_t __capacity = 1024 / sizeof(_CharT);

public:
  using value_type = _CharT;

  class __iterator {
  public:
    using difference_type = ptrdiff_t;
    using value_type      = _CharT;

    _LIBCPP_HIDE_FROM_ABI explicit __iterator(__print_buffer& __buffer) : __buffer_(std::addressof(__buffer)) {}

    _LIBCPP_HIDE_FROM_ABI __iterator& operator=(const _CharT& __value) {
      __buffer_->push_back(__value);
      return *this;
    }
    _LIBCPP_HIDE_FROM_ABI __iterator& operator=(_CharT&& __value) {
      __buffer_->push_back(__value);
      return *this;
    }

    _LIBCPP_HIDE_FROM_ABI __iterator& operator*() { return *this; }
    _LIBCPP_HIDE_FROM_ABI __iterator& operator++() { return *this; }
    _LIBCPP_HIDE_FROM_ABI __iterator operator++(int) { return *this; }

  private:
    __print_buffer* __buffer_;
  };

  __print_buffer(const __print_buffer&)            = delete;
  __print_buffer& operator=(const __print_buffer&) = delete;

  template <class _Fn>
  _LIBCPP_HIDE_FROM_ABI explicit __print_buffer(_Fn&& __flush) : __flush_(std::forward<_Fn>(__flush)) {}

  _LIBCPP_HIDE_FROM_ABI __iterator __make_output_iterator() { return __iterator{*this}; }

  _LIBCPP_HIDE_FROM_ABI void push_back(_CharT __value) {
    __buffer_[__size_++] = __value;
    if (__size_ == __capacity)
      __flush();
  }

  // I/O is deliberately explicit. A destructor cannot report a failed write.
  _LIBCPP_HIDE_FROM_ABI void __flush() {
    if (__size_ == 0)
      return;
    __flush_(__buffer_, __size_);
    __size_ = 0;
  }

private:
  _CharT __buffer_[__capacity];
  size_t __size_ = 0;
  _Flush __flush_;
};

} // namespace __format

#endif // _LIBCPP_STD_VER >= 23

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___FORMAT_PRINT_BUFFER_H
