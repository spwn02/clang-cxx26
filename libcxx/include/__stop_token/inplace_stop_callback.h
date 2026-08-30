// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___STOP_TOKEN_INPLACE_STOP_CALLBACK_H
#define _LIBCPP___STOP_TOKEN_INPLACE_STOP_CALLBACK_H

#include <__concepts/constructible.h>
#include <__concepts/destructible.h>
#include <__concepts/invocable.h>
#include <__config>
#include <__stop_token/inplace_stop_source.h>
#include <__stop_token/inplace_stop_token.h>
#include <__stop_token/stop_state.h>
#include <__type_traits/is_nothrow_constructible.h>
#include <__utility/forward.h>
#include <__utility/move.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_PUSH_MACROS
#include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_THREADS

// [stopcallback.inplace]
template <class _Callback>
class _LIBCPP_AVAILABILITY_NEW_SYNC inplace_stop_callback : private __stop_callback_base {
  static_assert(invocable<_Callback>,
                "Mandates: inplace_stop_callback is instantiated with an argument for the template parameter "
                "Callback that satisfies invocable.");
  static_assert(destructible<_Callback>,
                "Mandates: inplace_stop_callback is instantiated with an argument for the template parameter "
                "Callback that satisfies destructible.");

public:
  using callback_type = _Callback;

  template <class _Cb>
    requires constructible_from<_Callback, _Cb>
  _LIBCPP_HIDE_FROM_ABI explicit inplace_stop_callback(
      inplace_stop_token __token, _Cb&& __cb) noexcept(is_nothrow_constructible_v<_Callback, _Cb>)
      : __stop_callback_base([](__stop_callback_base* __cb_base) noexcept {
          // an inplace_stop_callback is only ever invoked once
          std::forward<_Callback>(static_cast<inplace_stop_callback*>(__cb_base)->__callback_)();
        }),
        __callback_(std::forward<_Cb>(__cb)),
        __source_(__token.__source_) {
    if (__source_ != nullptr && !__source_->__state_.__add_callback(this)) {
      // either the callback ran synchronously (stop already requested) or will never run (no
      // associated source's stop state to register against) — either way, nothing to
      // deregister in the destructor.
      __source_ = nullptr;
    }
  }

  _LIBCPP_HIDE_FROM_ABI ~inplace_stop_callback() {
    if (__source_ != nullptr) {
      __source_->__state_.__remove_callback(this);
    }
  }

  inplace_stop_callback(const inplace_stop_callback&)            = delete;
  inplace_stop_callback(inplace_stop_callback&&)                 = delete;
  inplace_stop_callback& operator=(const inplace_stop_callback&) = delete;
  inplace_stop_callback& operator=(inplace_stop_callback&&)      = delete;

private:
  _LIBCPP_NO_UNIQUE_ADDRESS _Callback __callback_;
  const inplace_stop_source* __source_;

  friend __stop_callback_base;
};

template <class _Callback>
_LIBCPP_AVAILABILITY_NEW_SYNC inplace_stop_callback(inplace_stop_token, _Callback) -> inplace_stop_callback<_Callback>;

#endif // _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_THREADS

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP___STOP_TOKEN_INPLACE_STOP_CALLBACK_H
