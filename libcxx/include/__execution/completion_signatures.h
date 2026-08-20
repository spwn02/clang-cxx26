//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___EXECUTION_COMPLETION_SIGNATURES_H
#define _LIBCPP___EXECUTION_COMPLETION_SIGNATURES_H

#include <__config>
#include <__execution/completion_functions.h>
#include <__type_traits/is_function.h>
#include <__type_traits/is_void.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

namespace execution {

// [exec.cmplsig]
// Exposition-only: `type-list` is a bare marker template, deliberately left without a
// body per the synopsis (`template<class... Ts> struct type-list;`) -- it's only ever
// used as a pattern-matched/compared type, never instantiated as an object.
template <class... _Ts>
struct type_list;

// A type `Fn` satisfies completion-signature iff it is a function type of one of the
// forms set_value_t(Vs...), set_error_t(Err), or set_stopped_t(), where Vs/Err are
// object-or-reference types. Mirrors the standard's own generic-lambda-plus-pointer-to-
// function-type idiom (see valid-completion-for in <__execution/receiver.h>) so the check
// stays a soft, per-overload SFINAE probe rather than an eager `requires{}` evaluation.
template <class... _Vs>
_LIBCPP_HIDE_FROM_ABI void __completion_signature_test(set_value_t (*)(_Vs...))
  requires((!is_function_v<_Vs> && !is_void_v<_Vs>) && ...);
template <class _Err>
_LIBCPP_HIDE_FROM_ABI void __completion_signature_test(set_error_t (*)(_Err))
  requires(!is_function_v<_Err> && !is_void_v<_Err>);
_LIBCPP_HIDE_FROM_ABI void __completion_signature_test(set_stopped_t (*)());

template <class _Fn>
concept __completion_signature = requires(_Fn* __fn) { execution::__completion_signature_test(__fn); };

template <__completion_signature... _Fns>
struct completion_signatures {};

template <class _Sigs>
inline constexpr bool __valid_completion_signatures_v = false;
template <class... _Fns>
inline constexpr bool __valid_completion_signatures_v<completion_signatures<_Fns...>> = true;

// [exec.cmplsig]: "a Sigs is valid-completion-signatures if it is a specialization of
// completion_signatures".
template <class _Sigs>
concept __valid_completion_signatures = __valid_completion_signatures_v<_Sigs>;

// Exposition-only META-APPLY machinery: makes it valid to use non-variadic templates as
// the Tuple/Variant arguments to gather-signatures.
template <bool>
struct __indirect_meta_apply {
  template <template <class...> class _Tp, class... _As>
  using __meta_apply = _Tp<_As...>;
};

template <class...>
concept __always_true = true;

template <template <class...> class _Tp, class... _As>
using __meta_apply = typename __indirect_meta_apply<__always_true<_As...>>::template __meta_apply<_Tp, _As...>;

// Per-signature step of gather-signatures: if `_Fn`'s return type is `_Tag`, wrap its
// argument list (via META-APPLY, so `_Tuple` need not be variadic) in a one-element
// `type_list`; otherwise contribute nothing.
template <class _Tag, class _Fn, template <class...> class _Tuple>
struct __gather_one {
  using type = type_list<>;
};
template <class _Tag, class... _Args, template <class...> class _Tuple>
struct __gather_one<_Tag, _Tag(_Args...), _Tuple> {
  using type = type_list<__meta_apply<_Tuple, _Args...>>;
};

template <class... _Ls>
struct __concat_type_lists {
  using type = type_list<>;
};
template <class... _Ts>
struct __concat_type_lists<type_list<_Ts...>> {
  using type = type_list<_Ts...>;
};
template <class... _Ts, class... _Us, class... _Rest>
struct __concat_type_lists<type_list<_Ts...>, type_list<_Us...>, _Rest...>
    : __concat_type_lists<type_list<_Ts..., _Us...>, _Rest...> {};

template <class _Tag, class _Completions, template <class...> class _Tuple, template <class...> class _Variant>
struct __gather_signatures_impl;

template <class _Tag, template <class...> class _Tuple, template <class...> class _Variant, class... _Fns>
struct __gather_signatures_impl<_Tag, completion_signatures<_Fns...>, _Tuple, _Variant> {
private:
  using __gathered = typename __concat_type_lists<typename __gather_one<_Tag, _Fns, _Tuple>::type...>::type;

  template <class>
  struct __apply_variant;
  template <class... _Ts>
  struct __apply_variant<type_list<_Ts...>> {
    using type = __meta_apply<_Variant, _Ts...>;
  };

public:
  using type = typename __apply_variant<__gathered>::type;
};

// [exec.cmplsig]: gather-signatures<Tag, Completions, Tuple, Variant>.
template <class _Tag,
          __valid_completion_signatures _Completions,
          template <class...> class _Tuple,
          template <class...> class _Variant>
using __gather_signatures = typename __gather_signatures_impl<_Tag, _Completions, _Tuple, _Variant>::type;

} // namespace execution

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___EXECUTION_COMPLETION_SIGNATURES_H
