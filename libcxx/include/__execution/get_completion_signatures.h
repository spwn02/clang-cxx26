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
#include <__execution/awaitable.h>
#include <__execution/completion_signatures.h>
#include <__execution/domain.h>
#include <__execution/env.h>
#include <__execution/queryable.h>
#include <__execution/sender.h>
#include <__type_traits/integral_constant.h>
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
// The Effects of get_completion_signatures fall through to throwing `dependent_sender_error` (for a sender whose
// completion signatures depend on the environment, when asked without one) or an unspecified exception derived from
// `exception` (for any other unusable sender), evaluated in constant evaluation. `sender_in` asks whether the call is a
// constant expression, and an exception that escapes constant evaluation makes it one that is not.
struct dependent_sender_error : exception {};

// [exec.snd.general]: the exception object of the other cases: a handler of type exception matches, a handler of type
// dependent_sender_error does not.
struct __unspecified_exception : exception {};

// [exec.getcomplsigs]: get-complsigs<Sndr, Env...>() is `remove_reference_t<Sndr>::template
// get_completion_signatures<Sndr, Env...>()`; whether that is well-formed does not depend on the type it returns.
template <class _Sndr, class... _Env>
concept __has_member_get_completion_signatures =
    requires { remove_reference_t<_Sndr>::template get_completion_signatures<_Sndr, _Env...>(); };

// [exec.snd.concepts]: SET-VALUE-SIG(T) -- set_value_t() if T is void, otherwise set_value_t(T).
// Partial specialization rather than conditional_t<is_void_v<_Tp>, set_value_t(), set_value_t(_Tp)>:
// conditional_t requires both alternatives to be well-formed types before picking one, and
// set_value_t(void) is ill-formed ("argument may not have 'void' type") regardless of which
// branch would ultimately be selected.
template <class _Tp>
struct __set_value_sig {
  using type = set_value_t(_Tp);
};
template <>
struct __set_value_sig<void> {
  using type = set_value_t();
};
template <class _Tp>
using __set_value_sig_t = typename __set_value_sig<_Tp>::type;

// [exec.getcomplsigs]: NewSndr is Sndr if sizeof...(Env) == 0; otherwise
// decltype(transform_sender(declval<Sndr>(), declval<Env>()...)).
template <class _Sndr>
_LIBCPP_HIDE_FROM_ABI auto __get_compl_sigs_new_sndr() -> _Sndr;
template <class _Sndr, class _Env>
_LIBCPP_HIDE_FROM_ABI auto __get_compl_sigs_new_sndr()
    -> decltype(execution::transform_sender(std::declval<_Sndr>(), std::declval<_Env>()));

template <class _Sndr, class... _Env>
using __get_compl_sigs_new_sndr_t = decltype(execution::__get_compl_sigs_new_sndr<_Sndr, _Env...>());

// [exec.getcomplsigs] (3.3): the awaitable fallback, reached only when neither member
// get_completion_signatures overload above is viable -- this disjunct is therefore only
// ever *evaluated* (never short-circuited away) for types that are already known not to be
// library-provided senders, so __is_awaitable's own SFINAE-safety (see awaitable.h) is what
// keeps this from hard-erroring on ordinary non-sender, non-awaitable types.
template <class _Sndr, class... _Env>
concept __get_compl_sigs_awaitable_fallback = __is_awaitable<_Sndr, __env_promise<_Env>...>;

// A constant expression of type `__constant_probe<expr>` can only be named if `expr` is a constant expression, which in
// a requires-expression is a substitution failure (and not a hard error) when it is not: it is the is-constant concept
// of [exec.snd.concepts] (a concept-id whose argument is unused is not substituted by this compiler, a class template-id is).
template <auto>
struct __constant_probe {};

// [exec.getcomplsigs]: CHECKED-COMPLSIGS(e) is e if e is a core constant expression whose type satisfies
// valid-completion-signatures, and (e, throw except, completion_signatures()) otherwise.
template <class _Sndr, class... _Env>
_LIBCPP_HIDE_FROM_ABI consteval auto __checked_complsigs() {
  using _Result = decltype(remove_reference_t<_Sndr>::template get_completion_signatures<_Sndr, _Env...>());
  if constexpr (__valid_completion_signatures<_Result>) {
    return remove_reference_t<_Sndr>::template get_completion_signatures<_Sndr, _Env...>();
  } else {
    (void)remove_reference_t<_Sndr>::template get_completion_signatures<_Sndr, _Env...>();
    throw __unspecified_exception();
    return completion_signatures<>();
  }
}

// [exec.getcomplsigs]
template <class _Sndr, class... _Env>
  requires(sizeof...(_Env) <= 1) && requires { typename __get_compl_sigs_new_sndr_t<_Sndr, _Env...>; }
consteval __valid_completion_signatures auto get_completion_signatures() {
  using _NewSndr = __get_compl_sigs_new_sndr_t<_Sndr, _Env...>;
  if constexpr (__has_member_get_completion_signatures<_NewSndr, _Env...>) {
    return execution::__checked_complsigs<_NewSndr, _Env...>();
  } else if constexpr (__has_member_get_completion_signatures<_NewSndr>) {
    return execution::__checked_complsigs<_NewSndr>();
  } else if constexpr (__get_compl_sigs_awaitable_fallback<_NewSndr, _Env...>) {
    using _Vp = __await_result_type<_NewSndr, __env_promise<_Env>...>;
    return completion_signatures<__set_value_sig_t<_Vp>, set_error_t(exception_ptr), set_stopped_t()>{};
  } else if constexpr (sizeof...(_Env) == 0) {
    throw dependent_sender_error();
    return completion_signatures<>();
  } else {
    throw __unspecified_exception();
    return completion_signatures<>();
  }
}

// [exec.snd.concepts]
template <class _Sndr, class... _Env>
concept sender_in =
    sender<_Sndr> && (sizeof...(_Env) <= 1) && (__queryable<_Env> && ...) &&
    requires { typename __constant_probe<execution::get_completion_signatures<_Sndr, _Env...>()>; };

// [exec.snd.concepts]: is-dependent-sender-helper and dependent_sender.
template <class _Sndr>
_LIBCPP_HIDE_FROM_ABI consteval bool __is_dependent_sender_helper() try {
  execution::get_completion_signatures<_Sndr>();
  return false;
} catch (dependent_sender_error&) {
  return true;
}

template <class _Sndr>
concept dependent_sender = sender<_Sndr> && requires {
  requires bool_constant<execution::__is_dependent_sender_helper<_Sndr>()>::value;
};

template <class _Sndr, class... _Env>
  requires sender_in<_Sndr, _Env...>
using completion_signatures_of_t = decltype(execution::get_completion_signatures<_Sndr, _Env...>());

// The completion signatures without an environment of a sender that transform_sender lowers to another one
// (bulk, starts_on, stopped_as_error, when_all_with_variant): those of the lowered sender, a dependent sender if the
// lowered sender is one.
template <class _Sndr>
_LIBCPP_HIDE_FROM_ABI consteval auto __lowered_signatures_without_env() {
  using _Lowered = decltype(execution::transform_sender(std::declval<_Sndr>(), std::declval<env<>>()));
  if constexpr (sender_in<_Lowered>) {
    return completion_signatures_of_t<_Lowered>{};
  } else {
    throw dependent_sender_error();
    return completion_signatures<>();
  }
}

// [exec.cmplsig]: decayed-tuple and variant-or-empty.
template <class... _Ts>
using __decayed_tuple = tuple<decay_t<_Ts>...>;

struct __empty_variant {
  __empty_variant() = delete;
};

template <class _List, class _Tp>
struct __type_list_append_unique {
  using type = _List;
};
template <class... _Ts, class _Tp>
  requires(!(is_same_v<_Ts, _Tp> || ...))
struct __type_list_append_unique<type_list<_Ts...>, _Tp> {
  using type = type_list<_Ts..., _Tp>;
};

template <class _List, class... _Ts>
struct __type_list_dedup {
  using type = _List;
};
template <class _List, class _Tp, class... _Rest>
struct __type_list_dedup<_List, _Tp, _Rest...>
    : __type_list_dedup<typename __type_list_append_unique<_List, _Tp>::type, _Rest...> {};

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
// (an invalid arity here is a hard error because it occurs during implicit class-template
// instantiation, outside the immediate context of the surrounding alias substitution). Working
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
