//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___EXECUTION_COMPLETION_ATTRS_H
#define _LIBCPP___EXECUTION_COMPLETION_ATTRS_H

#include <__concepts/same_as.h>
#include <__config>
#include <__execution/completion_signatures.h>
#include <__execution/domain.h>
#include <__execution/fwd_env.h>
#include <__execution/get_completion_signatures.h>
#include <__execution/get_scheduler.h>
#include <__type_traits/decay.h>
#include <__type_traits/is_nothrow_constructible.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/forward.h>
#include <__utility/move.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_PUSH_MACROS
#include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

namespace execution {

// [exec.snd.general]p9-10 (and its example with then): which completion operations of the child run on the same
// execution agents as the completion operations with tag O of an adaptor that intercepts the child's completions with
// tag _SetCpo, turns them into completions with tag _ToCpo (the adaptor's function result, or the datum it creates) and
// whose interception can throw (_MayThrow, an error completion that happens in the same place). The result is a bit set
// of the child's completion tags (1: value, 2: error, 4: stopped).
//   - a completion of the child with a tag other than the intercepted one is forwarded with the same tag;
//   - the interception happens where the child completed with the intercepted tag, and its result is a completion with
//     tag _ToCpo (so those completions of the adaptor can come from two places);
//   - if the interception can throw, the exception is an error completion that happens in the same place.
template <class _SetCpo, class _ToCpo, bool _MayThrow, class _ChildSigs>
struct __intercept_contributors {
  static constexpr bool __has_value   = !same_as<__exec_type_list<>, __gather_signatures<set_value_t, _ChildSigs, __exec_type_list, __exec_type_list>>;
  static constexpr bool __has_error   = !same_as<__exec_type_list<>, __gather_signatures<set_error_t, _ChildSigs, __exec_type_list, __exec_type_list>>;
  static constexpr bool __has_stopped = !same_as<__exec_type_list<>, __gather_signatures<set_stopped_t, _ChildSigs, __exec_type_list, __exec_type_list>>;

  template <class _Tg>
  static constexpr bool __has = same_as<_Tg, set_value_t>   ? __has_value
                              : same_as<_Tg, set_error_t>   ? __has_error
                                                            : __has_stopped;

  template <class _Tg>
  static constexpr unsigned __bit = same_as<_Tg, set_value_t> ? 1u : same_as<_Tg, set_error_t> ? 2u : 4u;

  template <class _Out>
  static constexpr unsigned __mask() {
    unsigned __m = 0;
    if constexpr (!same_as<_Out, _SetCpo>) {
      if constexpr (__has<_Out>)
        __m |= __bit<_Out>;
    }
    if constexpr (same_as<_Out, _ToCpo>) {
      if constexpr (__has<_SetCpo>)
        __m |= __bit<_SetCpo>;
    }
    if constexpr (same_as<_Out, set_error_t>) {
      if constexpr (_MayThrow && __has<_SetCpo>)
        __m |= __bit<_SetCpo>;
    }
    return __m;
  }
};

// The contributors of an adaptor that completes wherever its child completes, whatever the tag (schedule_from, the
// exposition-only stop-when): the completion queries are those of the child.
struct __identity_contrib {
  template <class _ChildSigs, class _Out>
  static consteval unsigned __mask() {
    return __intercept_contributors<void, void, false, _ChildSigs>::template __mask<_Out>();
  }
};

// Whether decay-copying the datums of a completion of a child with tag _Cpo can throw.
template <class _Cpo, class _Sig>
inline constexpr bool __sig_decay_copy_may_throw_v = false;
template <class _Cpo, class... _Ts>
inline constexpr bool __sig_decay_copy_may_throw_v<_Cpo, _Cpo(_Ts...)> = !(is_nothrow_constructible_v<decay_t<_Ts>, _Ts> && ...);

template <class _Cpo, class _Sigs>
inline constexpr bool __decay_copy_may_throw_v = false;
template <class _Cpo, class... _Sigs>
inline constexpr bool __decay_copy_may_throw_v<_Cpo, completion_signatures<_Sigs...>> =
    (__sig_decay_copy_may_throw_v<_Cpo, _Sigs> || ... || false);

// The contributors of an adaptor that decay-copies the result datums of the child's _SetCpo completion into its own
// result (into_variant, stopped_as_optional).
template <class _SetCpo, class _ToCpo>
struct __decay_copy_contrib {
  template <class _ChildSigs, class _Out>
  static consteval unsigned __mask() {
    return __intercept_contributors<_SetCpo, _ToCpo, __decay_copy_may_throw_v<_SetCpo, _ChildSigs>, _ChildSigs>::
        template __mask<_Out>();
  }
};

// The attributes of an adaptor with one child sender: those of the child, but for the completion queries, which are
// answered from the completion queries of the child as described by the contributors policy _Contrib (a type with a
// `template <class _ChildSigs, class _Out> static consteval unsigned __mask()`, see __intercept_contributors).
template <class _Contrib, class _Child, class _ChildAttrs>
class __completion_attrs {
private:
  _ChildAttrs __attrs_;

  template <class... _Envs>
  using __child_sigs_t =
      completion_signatures_of_t<_Child, __fwd_env<remove_cvref_t<_Envs>>...>;

  // The completion tags (as a bit set) whose agents the completions with tag _Cpo happen on, empty if that cannot be
  // determined (the signatures of the child need an environment, or the environment has no such query).
  template <class _Cpo, class... _Envs>
  static consteval unsigned __contributors() {
    if constexpr (sizeof...(_Envs) <= 1 && requires { typename __child_sigs_t<_Envs...>; } &&
                  (is_same_v<_Cpo, set_value_t> || is_same_v<_Cpo, set_error_t> || is_same_v<_Cpo, set_stopped_t>)) {
      return _Contrib::template __mask<__child_sigs_t<_Envs...>, _Cpo>();
    } else {
      return 0;
    }
  }

  template <unsigned _Mask, class _Query, class... _Envs>
  static consteval bool __child_answers() {
    return requires(const _ChildAttrs& __attrs, const _Envs&... __envs) {
      _Query()(__attrs, execution::__fwd_env_fn(__envs)...);
    };
  }

  // For a scheduler there must be exactly one place.
  template <class _Cpo, class... _Envs>
  static consteval unsigned __sched_plan() {
    constexpr unsigned __m = __contributors<_Cpo, _Envs...>();
    if constexpr (__m == 1)
      return __child_answers<1, get_completion_scheduler_t<set_value_t>, _Envs...>() ? 1 : 0;
    else if constexpr (__m == 2)
      return __child_answers<2, get_completion_scheduler_t<set_error_t>, _Envs...>() ? 2 : 0;
    else if constexpr (__m == 4)
      return __child_answers<4, get_completion_scheduler_t<set_stopped_t>, _Envs...>() ? 4 : 0;
    else
      return 0;
  }

  // For a domain: every place has to have one, and the result is their COMMON-DOMAIN.
  template <class _Cpo, class... _Envs>
  static consteval unsigned __domain_plan() {
    constexpr unsigned __m = __contributors<_Cpo, _Envs...>();
    if constexpr (__m == 0)
      return 0;
    else
      return ((__m & 1) == 0 || __child_answers<1, get_completion_domain_t<set_value_t>, _Envs...>()) &&
                     ((__m & 2) == 0 || __child_answers<2, get_completion_domain_t<set_error_t>, _Envs...>()) &&
                     ((__m & 4) == 0 || __child_answers<4, get_completion_domain_t<set_stopped_t>, _Envs...>())
                 ? __m
                 : 0;
  }

  template <class _Cpo, class... _Envs>
  _LIBCPP_HIDE_FROM_ABI constexpr auto __child_domain(const _Envs&... __envs) const noexcept {
    return execution::get_completion_domain<_Cpo>(__attrs_, execution::__fwd_env_fn(__envs)...);
  }

  template <unsigned _Mask, class... _Envs>
  _LIBCPP_HIDE_FROM_ABI constexpr auto __domain_of(const _Envs&... __envs) const noexcept {
    using _Val = set_value_t;
    using _Err = set_error_t;
    using _Stp = set_stopped_t;
    if constexpr (_Mask == 1)
      return __child_domain<_Val>(__envs...);
    else if constexpr (_Mask == 2)
      return __child_domain<_Err>(__envs...);
    else if constexpr (_Mask == 4)
      return __child_domain<_Stp>(__envs...);
    else if constexpr (_Mask == 3)
      return execution::__common_domain(__child_domain<_Val>(__envs...), __child_domain<_Err>(__envs...));
    else if constexpr (_Mask == 5)
      return execution::__common_domain(__child_domain<_Val>(__envs...), __child_domain<_Stp>(__envs...));
    else if constexpr (_Mask == 6)
      return execution::__common_domain(__child_domain<_Err>(__envs...), __child_domain<_Stp>(__envs...));
    else
      return execution::__common_domain(
          __child_domain<_Val>(__envs...), __child_domain<_Err>(__envs...), __child_domain<_Stp>(__envs...));
  }

public:
  _LIBCPP_HIDE_FROM_ABI constexpr explicit __completion_attrs(_ChildAttrs __attrs) noexcept(
      is_nothrow_move_constructible_v<_ChildAttrs>)
      : __attrs_(std::move(__attrs)) {}

  template <class _Query, class... _Args>
    requires(std::forwarding_query(_Query())) && (!__is_completion_query_v<_Query>) &&
            requires(const _ChildAttrs& __attrs, _Query __query, _Args&&... __args) {
              __attrs.query(__query, std::forward<_Args>(__args)...);
            }
  _LIBCPP_HIDE_FROM_ABI constexpr decltype(auto) query(_Query __query, _Args&&... __args) const
      noexcept(noexcept(std::declval<const _ChildAttrs&>().query(__query, std::forward<_Args>(__args)...))) {
    return __attrs_.query(__query, std::forward<_Args>(__args)...);
  }

  template <class _Cpo, class... _Envs>
    requires(__sched_plan<_Cpo, _Envs...>() != 0)
  _LIBCPP_HIDE_FROM_ABI constexpr auto query(get_completion_scheduler_t<_Cpo>, const _Envs&... __envs) const noexcept {
    constexpr unsigned __m = __sched_plan<_Cpo, _Envs...>();
    if constexpr (__m == 1)
      return execution::get_completion_scheduler<set_value_t>(__attrs_, execution::__fwd_env_fn(__envs)...);
    else if constexpr (__m == 2)
      return execution::get_completion_scheduler<set_error_t>(__attrs_, execution::__fwd_env_fn(__envs)...);
    else
      return execution::get_completion_scheduler<set_stopped_t>(__attrs_, execution::__fwd_env_fn(__envs)...);
  }

  template <class _Cpo, class... _Envs>
    requires(__domain_plan<_Cpo, _Envs...>() != 0)
  _LIBCPP_HIDE_FROM_ABI constexpr auto query(get_completion_domain_t<_Cpo>, const _Envs&... __envs) const noexcept {
    constexpr unsigned __m = __domain_plan<_Cpo, _Envs...>();
    return __domain_of<__m>(__envs...);
  }
};


} // namespace execution

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP___EXECUTION_COMPLETION_ATTRS_H
