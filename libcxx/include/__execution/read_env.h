//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___EXECUTION_READ_ENV_H
#define _LIBCPP___EXECUTION_READ_ENV_H

#include <__config>
#include <__execution/completion_functions.h>
#include <__execution/completion_signatures.h>
#include <__execution/get_env.h>
#include <__execution/movable_value.h>
#include <__execution/operation_state.h>
#include <__execution/sender.h>
#include <__type_traits/decay.h>
#include <__type_traits/is_nothrow_constructible.h>
#include <__type_traits/is_void.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/declval.h>
#include <__utility/forward.h>
#include <__utility/move.h>
#include <exception>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

namespace execution {

// [exec.read.env]
// Forward-declared so that __read_env_t (below) can name it in its operator()'s (uninstantiated, until
// called) return type before its full definition further down. read_env's own type is left unspecified by
// [execution.syn] (unlike just_t/schedule_t, which are named) -- __read_env_t is this fork's own name for
// it, not a standard one.
template <class _Query>
class __read_env_sndr;

template <class _Query, class _Rcvr>
class __read_env_opstate {
public:
  using operation_state_concept = operation_state_tag;

  _LIBCPP_HIDE_FROM_ABI constexpr __read_env_opstate(_Query&& __query, _Rcvr&& __rcvr)
      : __query_(std::move(__query)), __rcvr_(std::move(__rcvr)) {}

  __read_env_opstate(const __read_env_opstate&)            = delete;
  __read_env_opstate& operator=(const __read_env_opstate&) = delete;

  // [exec.read.env]p3: impls-for<read_env>::start is `[](auto query, auto& rcvr) noexcept -> void {
  // TRY-SET-VALUE(rcvr, query(get_env(rcvr))); }`. TRY-SET-VALUE(rcvr, expr) is TRY-EVAL(rcvr,
  // SET-VALUE(rcvr, expr)); TRY-EVAL wraps in try/catch, converting any exception to
  // set_error(std::move(rcvr), current_exception()), only if `expr` is potentially-throwing
  // ([exec.snd.expos]p11) -- computed below via the same noexcept(...) query call used to constrain
  // get_completion_signatures, so the two stay in lockstep.
  _LIBCPP_HIDE_FROM_ABI constexpr void start() & noexcept {
    if constexpr (noexcept(__query_(execution::get_env(__rcvr_)))) {
      execution::set_value(std::move(__rcvr_), __query_(execution::get_env(__rcvr_)));
    } else {
      try {
        execution::set_value(std::move(__rcvr_), __query_(execution::get_env(__rcvr_)));
      } catch (...) {
        execution::set_error(std::move(__rcvr_), std::current_exception());
      }
    }
  }

private:
  _Query __query_;
  _Rcvr __rcvr_;
};

struct __read_env_t {
  template <class _Query>
    requires __movable_value<_Query>
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Query&& __query) const
      noexcept(is_nothrow_constructible_v<decay_t<_Query>, _Query>) -> __read_env_sndr<decay_t<_Query>> {
    return __read_env_sndr<decay_t<_Query>>{{}, decay_t<_Query>(std::forward<_Query>(__query))};
  }
};

inline constexpr __read_env_t read_env{};

// An aggregate with public `tag`/`data` members, matching the (tag, data, ...children) shape that
// tag_of_t (<__execution/sender.h>) decomposes via structured bindings -- read_env has no child senders.
// Not routed through the draft's generic basic-sender/impls-for machinery: see the M3 entry in
// docs/CXX26_GAPS.md for why that engine isn't buildable on this fork yet.
template <class _Query>
class __read_env_sndr {
public:
  using sender_concept = sender_tag;

  _LIBCPP_NO_UNIQUE_ADDRESS __read_env_t tag;
  _Query data;

  template <class _Rcvr>
  _LIBCPP_HIDE_FROM_ABI constexpr auto connect(_Rcvr&& __rcvr) && -> __read_env_opstate<_Query, remove_cvref_t<_Rcvr>> {
    return __read_env_opstate<_Query, remove_cvref_t<_Rcvr>>(std::move(data), std::forward<_Rcvr>(__rcvr));
  }

  // [exec.read.env]p4-5: "Let Q be decay_t<data-type<Sndr>>. Throws: an exception ... if the expression
  // Q()(env) is ill-formed or has type void." This fork can't throw from a consteval function (M2
  // deviation 2, docs/CXX26_GAPS.md), so instead of hard-erroring/throwing, this overload simply doesn't
  // participate (via the nested `requires{}` below, checked in template-declaration substitution -- NOT
  // via body-instantiation, which M2's deviation 4 found to be a *hard* error outside "immediate context"
  // for this fork's Clang) when Q()(env) is ill-formed or void. That includes the zero-Env case: with no
  // Env supplied, `_Env` can't be deduced (this template has no function parameters to deduce it from) --
  // a plain, safely-SFINAE'd deduction failure -- so sender_in<read_env_sndr<Query>> (no Env) is false,
  // matching M2's documented "dependent-sender-as-soft-failure" deviation instead of reporting
  // dependent_sender<Sndr> as true.
  template <class _Self, class _Env>
    requires requires(const _Env& __env) {
      { _Query()(__env) };
      requires !is_void_v<decltype(_Query()(__env))>;
    }
  _LIBCPP_HIDE_FROM_ABI static consteval auto get_completion_signatures() {
    if constexpr (noexcept(_Query()(std::declval<const _Env&>()))) {
      return completion_signatures<set_value_t(decltype(_Query()(std::declval<const _Env&>())))>{};
    } else {
      return completion_signatures<set_value_t(decltype(_Query()(std::declval<const _Env&>()))), set_error_t(exception_ptr)>{};
    }
  }
};

} // namespace execution

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___EXECUTION_READ_ENV_H
