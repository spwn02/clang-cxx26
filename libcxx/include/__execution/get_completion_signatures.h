//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___EXECUTION_GET_COMPLETION_SIGNATURES_H
#define _LIBCPP___EXECUTION_GET_COMPLETION_SIGNATURES_H

#include <__concepts/same_as.h>
#include <__config>
#include <__execution/completion_signatures.h>
#include <__execution/domain.h>
#include <__execution/env.h>
#include <__execution/queryable.h>
#include <__execution/sender.h>
#include <__type_traits/decay.h>
#include <__type_traits/is_same.h>
#include <__type_traits/remove_reference.h>
#include <__type_traits/type_identity.h>
#include <__utility/declval.h>
#include <exception>
#include <tuple>
#include <variant>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

namespace execution {

// [exec.getcomplsigs]
// The standard's Effects clause can fall through to throwing `dependent_sender_error` (for
// a sender whose signatures genuinely depend on Env) or an unspecified `except` (any other
// unresolvable case), both evaluated at consteval time -- this requires throwing an
// exception from within a consteval function ([P3068]), which this Clang does not yet
// support (verified empirically: `consteval bool f() try { throw E{}; ... } catch (E&) {
// return true; }` fails to be a constant expression here). `dependent_sender_error` itself
// is declared regardless since downstream wording names it, but the throwing fallback
// paths are omitted: a sender with no viable get_completion_signatures dispatch (and that
// isn't itself awaitable, once M6 adds that) simply has no viable
// get_completion_signatures<Sndr, Env...>() overload, making sender_in false for it (the
// ordinary, non-dependent "not a sender_in" case) rather than hard-erroring -- see
// __has_completion_signatures below. The one real behavioral deviation this causes is for
// truly *dependent* senders (whose signatures can only be known once connected to a real
// environment, e.g. read_env's zero-env case in M3): rather than reporting
// `dependent_sender<Sndr>` as true, `sender_in<Sndr>` (zero-Env) will simply be false for
// them on this fork. Tracked as compiler-blocked, not scope-excluded, in
// docs/CXX26_GAPS.md.
struct dependent_sender_error : exception {};

template <class _Sndr, class... _Env>
concept __has_member_get_completion_signatures = requires {
  { remove_reference_t<_Sndr>::template get_completion_signatures<_Sndr, _Env...>() } -> __valid_completion_signatures;
};

// [exec.getcomplsigs]: NewSndr is Sndr if sizeof...(Env) == 0; otherwise
// decltype(transform_sender(declval<Sndr>(), declval<Env>()...)).
template <class _Sndr>
_LIBCPP_HIDE_FROM_ABI auto __get_compl_sigs_new_sndr() -> _Sndr;
template <class _Sndr, class _Env>
_LIBCPP_HIDE_FROM_ABI auto __get_compl_sigs_new_sndr()
    -> decltype(execution::transform_sender(std::declval<_Sndr>(), std::declval<_Env>()));

template <class _Sndr, class... _Env>
using __get_compl_sigs_new_sndr_t = decltype(execution::__get_compl_sigs_new_sndr<_Sndr, _Env...>());

// [exec.getcomplsigs]
template <class _Sndr, class... _Env>
  requires(sizeof...(_Env) <= 1) &&
          (__has_member_get_completion_signatures<__get_compl_sigs_new_sndr_t<_Sndr, _Env...>, _Env...> ||
           __has_member_get_completion_signatures<__get_compl_sigs_new_sndr_t<_Sndr, _Env...>>)
consteval __valid_completion_signatures auto get_completion_signatures() {
  using _NewSndr = __get_compl_sigs_new_sndr_t<_Sndr, _Env...>;
  if constexpr (__has_member_get_completion_signatures<_NewSndr, _Env...>) {
    return remove_reference_t<_NewSndr>::template get_completion_signatures<_NewSndr, _Env...>();
  } else {
    return remove_reference_t<_NewSndr>::template get_completion_signatures<_NewSndr>();
  }
}

template <class _Sndr, class... _Env>
concept sender_in =
    sender<_Sndr> && (sizeof...(_Env) <= 1) && (__queryable<_Env> && ...) &&
    requires { execution::get_completion_signatures<_Sndr, _Env...>(); };

template <class _Sndr, class... _Env>
  requires sender_in<_Sndr, _Env...>
using completion_signatures_of_t = decltype(execution::get_completion_signatures<_Sndr, _Env...>());

// [exec.cmplsig]: decayed-tuple and variant-or-empty.
template <class... _Ts>
using __decayed_tuple = tuple<decay_t<_Ts>...>;

struct __empty_variant {
  __empty_variant() = delete;
};

template <class _List, class _T>
struct __type_list_append_unique {
  using type = _List;
};
template <class... _Ts, class _T>
  requires(!(is_same_v<_Ts, _T> || ...))
struct __type_list_append_unique<type_list<_Ts...>, _T> {
  using type = type_list<_Ts..., _T>;
};

template <class _List, class... _Ts>
struct __type_list_dedup {
  using type = _List;
};
template <class _List, class _T, class... _Rest>
struct __type_list_dedup<_List, _T, _Rest...>
    : __type_list_dedup<typename __type_list_append_unique<_List, _T>::type, _Rest...> {};

template <class... _Ts>
using __dedup_type_list_t = typename __type_list_dedup<type_list<>, _Ts...>::type;

template <class... _Ts>
struct __variant_or_empty_impl {
  template <class>
  struct __to_variant;
  template <class... _Us>
  struct __to_variant<type_list<_Us...>> {
    using type = variant<_Us...>;
  };
  using type = typename __to_variant<__dedup_type_list_t<decay_t<_Ts>...>>::type;
};
template <>
struct __variant_or_empty_impl<> {
  using type = __empty_variant;
};

template <class... _Ts>
using __variant_or_empty = typename __variant_or_empty_impl<_Ts...>::type;

template <class _Sndr,
          class _Env                                = env<>,
          template <class...> class _Tuple          = __decayed_tuple,
          template <class...> class _Variant        = __variant_or_empty>
  requires sender_in<_Sndr, _Env>
using value_types_of_t = __gather_signatures<set_value_t, completion_signatures_of_t<_Sndr, _Env>, _Tuple, _Variant>;

template <class _Sndr, class _Env = env<>, template <class...> class _Variant = __variant_or_empty>
  requires sender_in<_Sndr, _Env>
using error_types_of_t =
    __gather_signatures<set_error_t, completion_signatures_of_t<_Sndr, _Env>, type_identity_t, _Variant>;

template <class _Sndr, class _Env = env<>>
  requires sender_in<_Sndr, _Env>
inline constexpr bool sends_stopped =
    !same_as<type_list<>, __gather_signatures<set_stopped_t, completion_signatures_of_t<_Sndr, _Env>, type_list, type_list>>;

// [exec.snd.expos]: single-sender-value-type<Sndr, Env> is the first of three alternatives that's
// well-formed: (1) gather-signatures<set_value_t, CS, decay_t, type_identity_t> -- exactly one
// set_value completion, with exactly one datum; (2) void, if every set_value completion (gathered via
// tuple/variant) has zero datums; (3) gather-signatures<set_value_t, CS, decayed-tuple, type_identity_t>
// -- exactly one set_value completion shape, tupled.
//
// Implemented against `value_types_of_t<Sndr, Env, __decayed_tuple, type_list>` -- a
// `type_list` of one decayed-tuple per set_value completion signature, always well-formed
// since __decayed_tuple (unlike decay_t) accepts any arity -- rather than gathering directly
// with `decay_t`/`type_identity_t` the way the standard's own (2.1) alternative literally
// spells it. `decay_t<Args...>` is ill-formed whenever a signature's arity isn't exactly one,
// and forming it happens *inside* __gather_one's implicit class-template instantiation
// (<__execution/completion_signatures.h>), not in the immediate context of any surrounding
// alias-template substitution -- so that failure is a hard error, not a SFINAE-droppable one
// (confirmed empirically: it turns out the "immediate context" pitfall this sub-plan's M1
// session flagged for `requires{}` simple-requirements on concrete objects also bites plain
// implicit class-template instantiation the same way, not just that one construct). Working
// entirely off the always-well-formed decayed-tuple gathering sidesteps this: every branch
// below forms its result via ordinary (SFINAE-safe) partial-specialization matching instead.
template <class _List>
struct __single_sender_value_type_impl {}; // more than one set_value shape: ill-formed, (2.4).

template <>
struct __single_sender_value_type_impl<type_list<>> {
  using type = void; // no set_value completion at all: (2.2)'s variant<> case.
};

template <class... _Args>
struct __single_sender_value_type_impl<type_list<tuple<_Args...>>> {
  using type = tuple<_Args...>; // (2.3): the single completion's decayed-tuple shape.
};

template <>
struct __single_sender_value_type_impl<type_list<tuple<>>> {
  using type = void; // (2.2)'s variant<tuple<>> case: the single completion has zero datums.
};

template <class _Arg>
struct __single_sender_value_type_impl<type_list<tuple<_Arg>>> {
  using type = _Arg; // (2.1): the single completion's single datum, unwrapped.
};

template <class _Sndr, class _Env = env<>>
  requires sender_in<_Sndr, _Env>
using __single_sender_value_type =
    typename __single_sender_value_type_impl<value_types_of_t<_Sndr, _Env, __decayed_tuple, type_list>>::type;

// [exec.snd.expos]: the exposition-only single-sender concept.
template <class _Sndr, class _Env = env<>>
concept __single_sender = sender_in<_Sndr, _Env> && requires { typename __single_sender_value_type<_Sndr, _Env>; };

} // namespace execution

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___EXECUTION_GET_COMPLETION_SIGNATURES_H
