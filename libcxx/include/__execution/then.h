//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___EXECUTION_THEN_H
#define _LIBCPP___EXECUTION_THEN_H

#include <__concepts/constructible.h>
#include <__config>
#include <__execution/completion_functions.h>
#include <__execution/completion_signatures.h>
#include <__execution/connect.h>
#include <__execution/fwd_env.h>
#include <__execution/get_completion_signatures.h>
#include <__execution/get_env.h>
#include <__execution/movable_value.h>
#include <__execution/operation_state.h>
#include <__execution/receiver.h>
#include <__execution/sender.h>
#include <__execution/sender_adaptor_closure.h>
#include <__functional/bind_back.h>
#include <__functional/invoke.h>
#include <__type_traits/conditional.h>
#include <__type_traits/decay.h>
#include <__type_traits/invoke.h>
#include <__type_traits/is_nothrow_constructible.h>
#include <__type_traits/is_same.h>
#include <__type_traits/is_void.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/forward.h>
#include <__utility/move.h>
#include <exception>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

namespace execution {

// [exec.then]. Only `then` itself is implemented here: `upon_error`/`upon_stopped` are the
// same mechanism (a `then-cpo` parameterized on which completion tag it intercepts) but are
// scoped to M5 in docs/CXX26_GAPS.md's sub-plan -- not built speculatively here since nothing
// in M4 needs them.

template <class _Fn, class _Sndr>
class __then_sndr;

// [exec.then]p3: then(sndr, f) is expression-equivalent to make-sender(then, f, sndr). Defined
// here, ahead of __then_sndr's own full definition below, so that __then_sndr can declare a
// `then_t tag;` data member of a *complete* type -- unlike <__execution/just.h>'s forward-
// declare-only trick (which works there because just_t is only ever named in `same_as<_Tag,
// ...>`-style comparisons inside a template, never as a data member's type), a non-static
// data member of a class *template* still needs its own type complete once that template is
// instantiated, and forward-declaring then_t alone doesn't get there. then_t::operator()
// itself only needs __then_sndr declared (forward-declared above), not complete, since its
// return type is deduced -- true for the same reason just_t/just_error_t's templated
// operator()s could reference __just_sndr before its definition.
struct then_t {
  template <sender _Sndr, __movable_value _Fn>
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Sndr&& __sndr, _Fn&& __fn) const
      -> __then_sndr<decay_t<_Fn>, remove_cvref_t<_Sndr>> {
    return __then_sndr<decay_t<_Fn>, remove_cvref_t<_Sndr>>{
        {}, decay_t<_Fn>(std::forward<_Fn>(__fn)), std::forward<_Sndr>(__sndr)};
  }

  template <class _Fn>
    requires constructible_from<decay_t<_Fn>, _Fn>
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Fn&& __fn) const
      noexcept(is_nothrow_constructible_v<decay_t<_Fn>, _Fn>) {
    return execution::__pipeable(std::__bind_back(*this, std::forward<_Fn>(__fn)));
  }
};

inline constexpr then_t then{};

template <class _Fn>
class __then_sig_transform {
public:
  // Non-value completion signatures pass through unchanged.
  template <class _Sig>
  struct __one {
    using type = type_list<_Sig>;
  };

  // A `void` specialization is required here, rather than picking between `set_value_t()`
  // and `set_value_t(_ResultT)` with __conditional_t/conditional_t once _ResultT is already
  // known to be `void`: those pick between two *already-formed* types, and forming
  // `set_value_t(_ResultT)` with `_ResultT = void` by substitution is a hard error ("argument
  // may not have 'void' type") -- unlike the literal, unsubstituted `F(void)` spelling in
  // source, which is specifically the "no parameters" idiom. Empirically confirmed in
  // isolation. A specialization on `_ResultT` sidesteps this: the primary template's
  // `set_value_t(_ResultT)` is simply never instantiated for `_ResultT = void`.
  template <class _ResultT>
  struct __then_value_sig {
    using type = set_value_t(_ResultT);
  };
  template <>
  struct __then_value_sig<void> {
    using type = set_value_t();
  };

  // [exec.then]p4's TRY-SET-VALUE(rcvr, invoke(fn, args...)): a value completion becomes
  // invoke_result_t<Fn, Ts...> (or no datum, if that's void), plus an exception_ptr error
  // completion unless the invocation is known not to throw.
  template <class... _Ts>
  struct __one<set_value_t(_Ts...)> {
    using __value_sig = typename __then_value_sig<invoke_result_t<_Fn, _Ts...>>::type;
    using type =
        __conditional_t<is_nothrow_invocable_v<_Fn, _Ts...>, type_list<__value_sig>,
                         type_list<__value_sig, set_error_t(exception_ptr)>>;
  };

  template <class _List>
  struct __dedup;
  template <class... _Ts>
  struct __dedup<type_list<_Ts...>> {
    using type = __dedup_type_list_t<_Ts...>;
  };

  template <class _List>
  struct __to_completion_signatures;
  template <class... _Sigs>
  struct __to_completion_signatures<type_list<_Sigs...>> {
    using type = completion_signatures<_Sigs...>;
  };

  template <class _Completions>
  struct __impl;
  template <class... _Fns>
  struct __impl<completion_signatures<_Fns...>> {
    using __gathered = typename __concat_type_lists<typename __one<_Fns>::type...>::type;
    using type        = typename __to_completion_signatures<typename __dedup<__gathered>::type>::type;
  };
};

template <class _Fn, class _Completions>
using __then_signatures_t = typename __then_sig_transform<_Fn>::template __impl<_Completions>::type;

// [exec.then]p4: the receiver that intercepts a value completion, invoking `fn` and routing
// its (possibly-throwing) result through TRY-SET-VALUE; every other completion forwards
// through to the outer receiver unchanged.
template <class _Fn, class _Rcvr>
class __then_rcvr {
public:
  using receiver_concept = receiver_tag;

  _LIBCPP_HIDE_FROM_ABI constexpr __then_rcvr(_Fn&& __fn, _Rcvr&& __rcvr)
      : __fn_(std::move(__fn)), __rcvr_(std::move(__rcvr)) {}

  template <class... _Args>
  _LIBCPP_HIDE_FROM_ABI constexpr void set_value(_Args&&... __args) && noexcept {
    using _Result = invoke_result_t<_Fn, _Args...>;
    if constexpr (is_nothrow_invocable_v<_Fn, _Args...>) {
      __complete(std::forward<_Args>(__args)...);
    } else {
      try {
        __complete(std::forward<_Args>(__args)...);
      } catch (...) {
        execution::set_error(std::move(__rcvr_), std::current_exception());
      }
    }
  }

  template <class _Err>
  _LIBCPP_HIDE_FROM_ABI constexpr void set_error(_Err&& __err) && noexcept {
    execution::set_error(std::move(__rcvr_), std::forward<_Err>(__err));
  }

  _LIBCPP_HIDE_FROM_ABI constexpr void set_stopped() && noexcept { execution::set_stopped(std::move(__rcvr_)); }

  _LIBCPP_HIDE_FROM_ABI constexpr auto get_env() const noexcept {
    return execution::__fwd_env_fn(execution::get_env(__rcvr_));
  }

private:
  template <class... _Args>
  _LIBCPP_HIDE_FROM_ABI constexpr void __complete(_Args&&... __args) {
    if constexpr (is_void_v<invoke_result_t<_Fn, _Args...>>) {
      std::invoke(std::move(__fn_), std::forward<_Args>(__args)...);
      execution::set_value(std::move(__rcvr_));
    } else {
      execution::set_value(std::move(__rcvr_), std::invoke(std::move(__fn_), std::forward<_Args>(__args)...));
    }
  }

  _Fn __fn_;
  _Rcvr __rcvr_;
};

// An aggregate with public `tag`/`data`/`child` members, matching the (tag, data,
// ...children) shape tag_of_t (<__execution/sender.h>) decomposes via structured bindings.
// Not routed through the draft's generic basic-sender/impls-for machinery: see the M3 entry
// in docs/CXX26_GAPS.md for why that engine isn't buildable on this fork yet.
template <class _Fn, class _Sndr>
class __then_sndr {
public:
  using sender_concept = sender_tag;

  _LIBCPP_NO_UNIQUE_ADDRESS then_t tag;
  _Fn data;
  _Sndr child;

  template <class _Rcvr>
  _LIBCPP_HIDE_FROM_ABI constexpr auto connect(_Rcvr&& __rcvr) && {
    return execution::connect(std::move(child), __then_rcvr<_Fn, remove_cvref_t<_Rcvr>>(
                                                      std::move(data), std::forward<_Rcvr>(__rcvr)));
  }

  // [exec.adapt.general]p3.2: a parent sender with a single child sndr has an associated
  // attribute object equal to FWD-ENV(get_env(sndr)).
  _LIBCPP_HIDE_FROM_ABI constexpr auto get_env() const noexcept {
    return execution::__fwd_env_fn(execution::get_env(child));
  }

  // [exec.then]p5 (check-types, the Mandates-throwing consteval helper) is not implemented --
  // same P3068 constexpr-exceptions gap as <__execution/get_completion_signatures.h>'s
  // dependent_sender_error (M2 deviation 2, docs/CXX26_GAPS.md); an `Fn` that isn't
  // invocable with the child's value datums simply makes this whole overload not
  // participate (via invoke_result_t/is_nothrow_invocable_v being ill-formed inside
  // __then_sig_transform), rather than reporting a dedicated diagnostic.
  //
  // `_Self` is accepted (matching [exec.getcomplsigs]'s call shape) but not used to vary
  // behavior, same as <__execution/just.h> and <__execution/read_env.h>: the child's
  // signatures are computed against FWD-ENV-T(Env) ([exec.then]p5), i.e. __fwd_env<Env>, to
  // match the environment the child is actually connected through at runtime
  // ([exec.adapt.general]p3.4). A single, non-variadic _Env parameter means the zero-Env
  // case simply has no viable overload -- the same "dependent-sender-as-soft-failure"
  // deviation <__execution/read_env.h> documents.
  template <class _Self, class _Env>
    requires sender_in<_Sndr, __fwd_env<remove_cvref_t<_Env>>>
  _LIBCPP_HIDE_FROM_ABI static consteval auto get_completion_signatures() {
    using __child_sigs = completion_signatures_of_t<_Sndr, __fwd_env<remove_cvref_t<_Env>>>;
    return __then_signatures_t<_Fn, __child_sigs>{};
  }
};

} // namespace execution

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___EXECUTION_THEN_H
