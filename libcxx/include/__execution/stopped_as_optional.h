//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___EXECUTION_STOPPED_AS_OPTIONAL_H
#define _LIBCPP___EXECUTION_STOPPED_AS_OPTIONAL_H

#include <__concepts/same_as.h>
#include <__config>
#include <__execution/fwd_env.h>
#include <__execution/get_completion_signatures.h>
#include <__execution/get_env.h>
#include <__execution/just.h>
#include <__execution/let.h>
#include <__execution/sender.h>
#include <__execution/sender_adaptor_closure.h>
#include <__execution/then.h>
#include <__type_traits/is_nothrow_constructible.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/declval.h>
#include <__utility/forward.h>
#include <__utility/in_place.h>
#include <__utility/move.h>
#include <optional>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

namespace execution {

// [exec.stopped.opt]. stopped_as_optional(sndr) maps sndr's stopped completion into a
// disengaged optional<V> and its (single) value completion into an engaged one, where V is
// single-sender-value-type<child, Env> (<__execution/get_completion_signatures.h>'s
// __single_sender_value_type). Unlike <__execution/stopped_as_error.h>'s and
// <__execution/starts_on.h>'s call-time compositions -- neither of which needs Env to build
// its result -- V genuinely depends on Env, which isn't known until connect()/
// get_completion_signatures() see a real receiver/queried environment. So this needs its own
// hand-rolled sender type: pin V once Env is available, then build and
// connect()/get_completion_signatures() against `let_stopped(then(child, value-lambda<V>),
// stopped-lambda<V>)` -- [exec.stopped.opt]p3's transform_sender body, verbatim, parameterized
// on the pinned V. Not routed through the draft's generic basic-sender/impls-for/make-sender
// machinery: see the M3 entry in docs/CXX26_GAPS.md for why that engine isn't buildable on
// this fork yet.
//
// [exec.stopped.opt]p3's check-types (Mandates: single-sender-value-type<child, Env> is not
// void) is not implemented -- same P3068 constexpr-exceptions gap as every other adaptor in
// this sub-plan; a child sender whose single value type is void, or that isn't a
// __single_sender at all, simply makes get_completion_signatures's overload not participate
// (sender_in false) rather than reporting a dedicated diagnostic.
//
// stopped_as_optional_t is defined (in full) below, *before* __stopped_as_optional_sndr --
// matching <__execution/write_env.h>'s own __write_env_t/__write_env_sndr ordering, for the
// same reason: __stopped_as_optional_sndr's `tag` member names stopped_as_optional_t directly
// and non-dependently (there's no `_Tag` template parameter here the way
// then_t/let_value_t/on_t have, since stopped_as_optional has exactly one CPO), and a
// non-dependent by-value member must be a complete type at the point its enclosing class
// *template* is defined, not deferred to instantiation. __stopped_as_optional_sndr itself is
// only forward-declared up front so that stopped_as_optional_t::operator()'s trailing return
// type can name it.
template <class _Sndr>
class __stopped_as_optional_sndr;

struct stopped_as_optional_t : sender_adaptor_closure<stopped_as_optional_t> {
  template <sender _Sndr>
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Sndr&& __sndr) const
      -> __stopped_as_optional_sndr<remove_cvref_t<_Sndr>>;
};

inline constexpr stopped_as_optional_t stopped_as_optional{};

template <class _Sndr>
class __stopped_as_optional_sndr {
public:
  using sender_concept = sender_tag;

  _LIBCPP_NO_UNIQUE_ADDRESS stopped_as_optional_t tag;
  _Sndr child;

  // [exec.stopped.opt]p3's transform_sender body, parameterized on the pinned value type `_V`
  // (computed by each caller below, once Env is known) rather than recomputed here.
  template <class _V, class _ChildSndr>
  _LIBCPP_HIDE_FROM_ABI static constexpr auto __compose(_ChildSndr&& __c) {
    return execution::let_stopped(
        execution::then(std::forward<_ChildSndr>(__c),
                         []<class... _Ts>(_Ts&&... __ts) noexcept(is_nothrow_constructible_v<_V, _Ts...>) {
                           return optional<_V>(in_place, std::forward<_Ts>(__ts)...);
                         }),
        []() noexcept { return execution::just(optional<_V>()); });
  }

  template <class _Rcvr>
  _LIBCPP_HIDE_FROM_ABI constexpr auto connect(_Rcvr&& __rcvr) && {
    using __env_t = __fwd_env<env_of_t<remove_cvref_t<_Rcvr>>>;
    using __v_t   = __single_sender_value_type<_Sndr, __env_t>;
    return execution::connect(__compose<__v_t>(std::move(child)), std::forward<_Rcvr>(__rcvr));
  }

  // [exec.adapt.general]p3.2: a parent sender with a single child sndr has an associated
  // attribute object equal to FWD-ENV(get_env(sndr)).
  _LIBCPP_HIDE_FROM_ABI constexpr auto get_env() const noexcept {
    return execution::__fwd_env_fn(execution::get_env(child));
  }

  template <class _Self, class _Env>
    requires __single_sender<_Sndr, __fwd_env<remove_cvref_t<_Env>>> &&
             (!same_as<void, __single_sender_value_type<_Sndr, __fwd_env<remove_cvref_t<_Env>>>>)
  _LIBCPP_HIDE_FROM_ABI static consteval auto get_completion_signatures() {
    using __env_t = __fwd_env<remove_cvref_t<_Env>>;
    using __v_t   = __single_sender_value_type<_Sndr, __env_t>;
    return completion_signatures_of_t<decltype(__compose<__v_t>(std::declval<_Sndr>())), __env_t>{};
  }
};

template <sender _Sndr>
_LIBCPP_HIDE_FROM_ABI constexpr auto stopped_as_optional_t::operator()(_Sndr&& __sndr) const
    -> __stopped_as_optional_sndr<remove_cvref_t<_Sndr>> {
  return __stopped_as_optional_sndr<remove_cvref_t<_Sndr>>{{}, std::forward<_Sndr>(__sndr)};
}

} // namespace execution

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___EXECUTION_STOPPED_AS_OPTIONAL_H
