// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___STOP_TOKEN_STOPPABLE_TOKEN_H
#define _LIBCPP___STOP_TOKEN_STOPPABLE_TOKEN_H

#include <__concepts/copyable.h>
#include <__concepts/equality_comparable.h>
#include <__concepts/same_as.h>
#include <__config>
#include <__type_traits/integral_constant.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_THREADS

// [stoptoken.concepts]
template <class _Token>
concept stoppable_token =
    copyable<_Token> && equality_comparable<_Token> && requires(const _Token& __token) {
      typename _Token::template callback_type<int>;
      { __token.stop_requested() } noexcept -> same_as<bool>;
      { __token.stop_possible() } noexcept -> same_as<bool>;
      { _Token(__token) } noexcept;
    };

template <class _Token>
concept unstoppable_token =
    stoppable_token<_Token> && requires(const _Token& __token) {
      // Forces `stop_possible()` to be a core constant expression that evaluates to `false`
      // (as opposed to merely being callable) — the whole point of this refinement is to let
      // callers statically skip callback registration for tokens that can never stop.
      requires bool_constant<!__token.stop_possible()>::value;
    };

// [thread.stoptoken.syn]: stop_callback_for_t<T, CallbackFn> is T::callback_type<CallbackFn> --
// the concrete stop-callback type a given stoppable_token uses, parameterized on the callback
// function object's type. Declared alongside stoppable_token/unstoppable_token per the
// synopsis (all three live in plain namespace std, not std::execution) rather than in its own
// header, since it has no machinery of its own beyond the alias.
template <class _Token, class _CallbackFn>
using stop_callback_for_t = typename _Token::template callback_type<_CallbackFn>;

#endif // _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_THREADS

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___STOP_TOKEN_STOPPABLE_TOKEN_H
