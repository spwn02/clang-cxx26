//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___EXECUTION_READ_ENV_H
#define _LIBCPP___EXECUTION_READ_ENV_H

#include <__concepts/constructible.h>
#include <__config>
#include <__execution/completion_functions.h>
#include <__execution/completion_signatures.h>
#include <__execution/fwd_env.h>
#include <__execution/get_completion_signatures.h>
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
#include <__utility/forward_like.h>
#include <__utility/move.h>
#include <exception>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_PUSH_MACROS
#include <__undef_macros>

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
// The sender directly stores its query tag and provides its own connection and signature logic.
template <class _Query>
class __read_env_sndr {
public:
  using sender_concept = sender_tag;

  _LIBCPP_NO_UNIQUE_ADDRESS __read_env_t tag;
  _Query data;

  template <class _Rcvr>
  _LIBCPP_HIDE_FROM_ABI constexpr auto connect(_Rcvr&& __rcvr) && noexcept(
      noexcept(__connect_with(std::move(*this), std::declval<_Rcvr>()))) {
    return __connect_with(std::move(*this), std::forward<_Rcvr>(__rcvr));
  }

  template <class _Rcvr>
    requires copy_constructible<_Query>
  _LIBCPP_HIDE_FROM_ABI constexpr auto connect(_Rcvr&& __rcvr) const& noexcept(
      noexcept(__connect_with(*this, std::declval<_Rcvr>()))) {
    return __connect_with(*this, std::forward<_Rcvr>(__rcvr));
  }

private:
  template <class _Self, class _Rcvr>
  _LIBCPP_HIDE_FROM_ABI static constexpr auto __connect_with(_Self&& __self, _Rcvr&& __rcvr)
      noexcept(is_nothrow_constructible_v<_Query, decltype(std::forward_like<_Self>(__self.data))>)
          -> __read_env_opstate<_Query, remove_cvref_t<_Rcvr>> {
    return __read_env_opstate<_Query, remove_cvref_t<_Rcvr>>(
        _Query(std::forward_like<_Self>(__self.data)), std::forward<_Rcvr>(__rcvr));
  }

public:

  // [exec.affine] (recommended): read_env completes in its start operation, on the agent it was started on.
  _LIBCPP_HIDE_FROM_ABI constexpr __read_env_sndr affine() && noexcept(is_nothrow_move_constructible_v<__read_env_sndr>) {
    return std::move(*this);
  }
  _LIBCPP_HIDE_FROM_ABI constexpr __read_env_sndr affine() const& noexcept(is_nothrow_copy_constructible_v<__read_env_sndr>)
    requires copy_constructible<__read_env_sndr>
  {
    return *this;
  }

  // [exec.read.env]p4-5 (check-types): "Let Q be decay_t<data-type<Sndr>>. Throws: an exception of type
  // unspecified-exception if the expression Q()(env) is ill-formed or has type void." This overload does not
  // throw the exception in that case. Like for every sender made with make-sender, the environment is the first of
  // `Env..., env<>` ([exec.snd.expos] basic-sender::get_completion_signatures): without an environment the query is
  // asked of env<>{}.
  template <class _Self, class... _Env>
  _LIBCPP_HIDE_FROM_ABI static consteval auto get_completion_signatures() {
    using __env_t = typename __first_env_or<env<>, remove_cvref_t<_Env>...>::type;
    if constexpr (!requires(const __env_t& __env) {
                    { _Query()(__env) };
                    requires !is_void_v<decltype(_Query()(__env))>;
                  }) {
      throw __unspecified_exception();
      return completion_signatures<>();
    } else if constexpr (noexcept(_Query()(std::declval<const __env_t&>()))) {
      return completion_signatures<set_value_t(decltype(_Query()(std::declval<const __env_t&>())))>{};
    } else {
      return completion_signatures<set_value_t(decltype(_Query()(std::declval<const __env_t&>()))),
                                   set_error_t(exception_ptr)>{};
    }
  }
};

} // namespace execution

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP___EXECUTION_READ_ENV_H
