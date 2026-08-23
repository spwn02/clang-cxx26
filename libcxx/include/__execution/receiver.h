//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___EXECUTION_RECEIVER_H
#define _LIBCPP___EXECUTION_RECEIVER_H

#include <__concepts/constructible.h>
#include <__concepts/derived_from.h>
#include <__concepts/invocable.h>
#include <__concepts/movable.h>
#include <__concepts/same_as.h>
#include <__config>
#include <__execution/completion_signatures.h>
#include <__execution/get_env.h>
#include <__execution/queryable.h>
#include <__type_traits/is_nothrow_constructible.h>
#include <__type_traits/remove_cvref.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

namespace execution {

// [exec.recv.concepts]
struct receiver_tag {};

template <class _Rcvr>
concept receiver =
    derived_from<typename remove_cvref_t<_Rcvr>::receiver_concept, receiver_tag> &&
    requires(const remove_cvref_t<_Rcvr>& __rcvr) {
      { execution::get_env(__rcvr) } -> __queryable;
    } && move_constructible<remove_cvref_t<_Rcvr>> && constructible_from<remove_cvref_t<_Rcvr>, _Rcvr> &&
    is_nothrow_move_constructible_v<remove_cvref_t<_Rcvr>>;

// Mirrors the standard's own generic-lambda-plus-pointer-to-function-type idiom so this
// stays a soft, per-instantiation SFINAE probe (see the eager-`requires{}`-evaluation
// finding recorded for M1 in docs/CXX26_GAPS.md: this pattern is required whenever the
// entities under test are otherwise concrete/non-dependent). The draft's own wording
// constrains this with a concept spelled `callable<Tag, remove_cvref_t<Rcvr>, Args...>`
// that does not appear defined anywhere in <concepts> or [exec]; `invocable` is used here
// as the closest standard equivalent (checking that `Tag{}(rcvr, args...)` is callable).
template <class _Signature, class _Rcvr>
concept __valid_completion_for = requires(_Signature* __sig) {
  []<class _Tag, class... _Args>(_Tag (*)(_Args...))
    requires invocable<_Tag, remove_cvref_t<_Rcvr>, _Args...>
  {}(__sig);
};

template <class _Rcvr, class _Completions>
concept __has_completions = requires(_Completions* __completions) {
  []<__valid_completion_for<_Rcvr>... _Sigs>(completion_signatures<_Sigs...>*) {}(__completions);
};

template <class _Rcvr, class _Completions>
concept receiver_of = receiver<_Rcvr> && __has_completions<_Rcvr, _Completions>;

} // namespace execution

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___EXECUTION_RECEIVER_H
