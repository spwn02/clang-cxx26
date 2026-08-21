//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___EXECUTION_JUST_H
#define _LIBCPP___EXECUTION_JUST_H

#include <__concepts/same_as.h>
#include <__config>
#include <__execution/completion_functions.h>
#include <__execution/completion_signatures.h>
#include <__execution/movable_value.h>
#include <__execution/operation_state.h>
#include <__execution/sender.h>
#include <__type_traits/decay.h>
#include <__type_traits/is_nothrow_constructible.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/forward.h>
#include <__utility/move.h>
#include <tuple>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

namespace execution {

// [exec.just]
// just_t/just_error_t/just_stopped_t are forward-declared (not yet defined) here so that __just_opstate and
// __just_sndr below -- both templates -- can name them in deferred (instantiated-on-use) contexts; neither
// needs them complete, only declared, since they're only ever used as plain types in `same_as<_Tag, ...>`
// comparisons. __just_sndr's *full* definition, in turn, must appear before just_t/just_error_t/
// just_stopped_t's own definitions (not after, as a first attempt at this file had it): just_stopped_t's
// `operator()` takes no template parameters, so -- unlike just_t's and just_error_t's -- its body isn't a
// template and is processed as an ordinary "complete-class context" (deferred only to just_stopped_t's own
// closing brace, not to first call), so __just_sndr<just_stopped_t> must already be a complete type by then.
struct just_t;
struct just_error_t;
struct just_stopped_t;

template <class _Tag, class... _Ts>
class __just_sndr;

template <class _Tag, class _Rcvr, class... _Ts>
class __just_opstate {
public:
  using operation_state_concept = operation_state_tag;

  _LIBCPP_HIDE_FROM_ABI constexpr __just_opstate(_Rcvr&& __rcvr, tuple<_Ts...>&& __data)
      : __rcvr_(std::move(__rcvr)), __data_(std::move(__data)) {}

  __just_opstate(const __just_opstate&)            = delete;
  __just_opstate& operator=(const __just_opstate&) = delete;

  // [exec.just]p2: impls-for<just-cpo>::start is `[](auto& state, auto& rcvr) noexcept -> void { auto&
  // [...ts] = state; set-cpo(std::move(rcvr), std::move(ts)...); }` -- unconditionally noexcept, since
  // forwarding std::move(ts)... doesn't itself invoke any potentially-throwing constructor (that's the
  // receiver's own responsibility, already covered by set_value_t/set_error_t/set_stopped_t's noexcept
  // Mandates in <__execution/completion_functions.h>).
  _LIBCPP_HIDE_FROM_ABI constexpr void start() & noexcept {
    auto& [...__ts] = __data_;
    if constexpr (same_as<_Tag, just_t>) {
      execution::set_value(std::move(__rcvr_), std::move(__ts)...);
    } else if constexpr (same_as<_Tag, just_error_t>) {
      execution::set_error(std::move(__rcvr_), std::move(__ts)...);
    } else {
      execution::set_stopped(std::move(__rcvr_));
    }
  }

private:
  _Rcvr __rcvr_;
  tuple<_Ts...> __data_;
};

// An aggregate with public `tag`/`data` members, matching the (tag, data, ...children) shape that
// tag_of_t (<__execution/sender.h>) decomposes via structured bindings -- just/just_error/just_stopped have
// no child senders. Not routed through the draft's generic basic-sender/impls-for machinery: see the M3
// entry in docs/CXX26_GAPS.md for why that engine isn't buildable on this fork yet.
template <class _Tag, class... _Ts>
class __just_sndr {
public:
  using sender_concept = sender_tag;

  _LIBCPP_NO_UNIQUE_ADDRESS _Tag tag;
  tuple<_Ts...> data;

  template <class _Rcvr>
  _LIBCPP_HIDE_FROM_ABI constexpr auto connect(_Rcvr&& __rcvr) && -> __just_opstate<_Tag, remove_cvref_t<_Rcvr>, _Ts...> {
    return __just_opstate<_Tag, remove_cvref_t<_Rcvr>, _Ts...>(std::forward<_Rcvr>(__rcvr), std::move(data));
  }

  // Env-independent (just/just_error/just_stopped's completions never depend on the receiver's
  // environment), so this participates unconditionally -- no SFINAE gating needed here, unlike read_env.
  template <class _Self, class... _Env>
  _LIBCPP_HIDE_FROM_ABI static consteval auto get_completion_signatures() {
    if constexpr (same_as<_Tag, just_t>) {
      return completion_signatures<set_value_t(_Ts...)>{};
    } else if constexpr (same_as<_Tag, just_error_t>) {
      return completion_signatures<set_error_t(_Ts...)>{};
    } else {
      return completion_signatures<set_stopped_t()>{};
    }
  }
};

struct just_t {
  template <class... _Ts>
    requires(__movable_value<_Ts> && ...)
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Ts&&... __ts) const
      noexcept((is_nothrow_constructible_v<decay_t<_Ts>, _Ts> && ...)) -> __just_sndr<just_t, decay_t<_Ts>...> {
    return __just_sndr<just_t, decay_t<_Ts>...>{{}, tuple<decay_t<_Ts>...>(std::forward<_Ts>(__ts)...)};
  }
};

struct just_error_t {
  template <class _Ts>
    requires __movable_value<_Ts>
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Ts&& __ts) const
      noexcept(is_nothrow_constructible_v<decay_t<_Ts>, _Ts>) -> __just_sndr<just_error_t, decay_t<_Ts>> {
    return __just_sndr<just_error_t, decay_t<_Ts>>{{}, tuple<decay_t<_Ts>>(std::forward<_Ts>(__ts))};
  }
};

struct just_stopped_t {
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()() const noexcept -> __just_sndr<just_stopped_t> {
    return __just_sndr<just_stopped_t>{{}, tuple<>()};
  }
};

inline constexpr just_t just{};
inline constexpr just_error_t just_error{};
inline constexpr just_stopped_t just_stopped{};

} // namespace execution

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___EXECUTION_JUST_H
