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
#include <__concepts/same_as.h>
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

// [exec.then]. `then`, `upon_error`, and `upon_stopped` are one clause in the standard --
// the same "intercept one completion tag, turn its datums into a value completion via `fn`,
// forward everything else unchanged" mechanism, parameterized on which completion tag is
// intercepted ([exec.then]p4's `set-cpo`). Modeled on <__execution/just.h>'s `_Tag`-templated
// shape (forward-declare the three CPO tag types, define the generic sender/receiver/signature
// machinery against a `_Tag` template parameter, then define the concrete CPO structs) rather
// than three near-duplicate files.
struct then_t;
struct upon_error_t;
struct upon_stopped_t;

template <class _Tag, class _Fn, class _Sndr>
class __then_sndr;

// [exec.then]p4's `set-cpo`: which completion-function tag (set_value_t/set_error_t/
// set_stopped_t) `_Tag` (then_t/upon_error_t/upon_stopped_t) intercepts.
template <class _Tag>
using __then_set_cpo_t =
    __conditional_t<same_as<_Tag, then_t>,
                     set_value_t,
                     __conditional_t<same_as<_Tag, upon_error_t>, set_error_t, set_stopped_t>>;

template <class _SetCpo, class _Fn>
class __then_sig_transform {
public:
  // Non-intercepted completion signatures pass through unchanged.
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

  // [exec.then]p4's TRY-SET-VALUE(rcvr, invoke(fn, args...)): the intercepted completion
  // becomes invoke_result_t<Fn, Args...> (or no datum, if that's void), plus an
  // exception_ptr error completion unless the invocation is known not to throw.
  template <class... _Args>
  struct __one<_SetCpo(_Args...)> {
    using __value_sig = typename __then_value_sig<invoke_result_t<_Fn, _Args...>>::type;
    using type =
        __conditional_t<is_nothrow_invocable_v<_Fn, _Args...>, type_list<__value_sig>,
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

template <class _Tag, class _Fn, class _Completions>
using __then_signatures_t =
    typename __then_sig_transform<__then_set_cpo_t<_Tag>, _Fn>::template __impl<_Completions>::type;

// [exec.then]p4: the receiver that intercepts `_Tag`'s completion tag, invoking `fn` and
// routing its (possibly-throwing) result through TRY-SET-VALUE; every other completion
// forwards through to the outer receiver unchanged.
template <class _Tag, class _Fn, class _Rcvr>
class __then_rcvr {
public:
  using receiver_concept = receiver_tag;

  _LIBCPP_HIDE_FROM_ABI constexpr __then_rcvr(_Fn&& __fn, _Rcvr&& __rcvr)
      : __fn_(std::move(__fn)), __rcvr_(std::move(__rcvr)) {}

  template <class... _Args>
  _LIBCPP_HIDE_FROM_ABI constexpr void set_value(_Args&&... __args) && noexcept {
    if constexpr (same_as<_Tag, then_t>) {
      __complete(std::forward<_Args>(__args)...);
    } else {
      execution::set_value(std::move(__rcvr_), std::forward<_Args>(__args)...);
    }
  }

  template <class _Err>
  _LIBCPP_HIDE_FROM_ABI constexpr void set_error(_Err&& __err) && noexcept {
    if constexpr (same_as<_Tag, upon_error_t>) {
      __complete(std::forward<_Err>(__err));
    } else {
      execution::set_error(std::move(__rcvr_), std::forward<_Err>(__err));
    }
  }

  _LIBCPP_HIDE_FROM_ABI constexpr void set_stopped() && noexcept {
    if constexpr (same_as<_Tag, upon_stopped_t>) {
      __complete();
    } else {
      execution::set_stopped(std::move(__rcvr_));
    }
  }

  _LIBCPP_HIDE_FROM_ABI constexpr auto get_env() const noexcept {
    return execution::__fwd_env_fn(execution::get_env(__rcvr_));
  }

private:
  template <class... _Args>
  _LIBCPP_HIDE_FROM_ABI constexpr void __complete(_Args&&... __args) {
    if constexpr (is_nothrow_invocable_v<_Fn, _Args...>) {
      __invoke_and_set_value(std::forward<_Args>(__args)...);
    } else {
      try {
        __invoke_and_set_value(std::forward<_Args>(__args)...);
      } catch (...) {
        execution::set_error(std::move(__rcvr_), std::current_exception());
      }
    }
  }

  template <class... _Args>
  _LIBCPP_HIDE_FROM_ABI constexpr void __invoke_and_set_value(_Args&&... __args) {
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
template <class _Tag, class _Fn, class _Sndr>
class __then_sndr {
public:
  using sender_concept = sender_tag;

  _LIBCPP_NO_UNIQUE_ADDRESS _Tag tag;
  _Fn data;
  _Sndr child;

  template <class _Rcvr>
  _LIBCPP_HIDE_FROM_ABI constexpr auto connect(_Rcvr&& __rcvr) && {
    return execution::connect(std::move(child), __then_rcvr<_Tag, _Fn, remove_cvref_t<_Rcvr>>(
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
  // invocable with the intercepted datums simply makes this whole overload not participate
  // (via invoke_result_t/is_nothrow_invocable_v being ill-formed inside __then_sig_transform),
  // rather than reporting a dedicated diagnostic.
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
    return __then_signatures_t<_Tag, _Fn, __child_sigs>{};
  }
};

template <class _Tag, class _Sndr, class _Fn>
_LIBCPP_HIDE_FROM_ABI constexpr auto __then_make_sndr(_Sndr&& __sndr, _Fn&& __fn) {
  return __then_sndr<_Tag, decay_t<_Fn>, remove_cvref_t<_Sndr>>{
      {}, decay_t<_Fn>(std::forward<_Fn>(__fn)), std::forward<_Sndr>(__sndr)};
}

struct then_t {
  template <sender _Sndr, __movable_value _Fn>
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Sndr&& __sndr, _Fn&& __fn) const
      -> __then_sndr<then_t, decay_t<_Fn>, remove_cvref_t<_Sndr>> {
    return execution::__then_make_sndr<then_t>(std::forward<_Sndr>(__sndr), std::forward<_Fn>(__fn));
  }

  template <class _Fn>
    requires constructible_from<decay_t<_Fn>, _Fn>
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Fn&& __fn) const
      noexcept(is_nothrow_constructible_v<decay_t<_Fn>, _Fn>) {
    return execution::__pipeable(std::__bind_back(*this, std::forward<_Fn>(__fn)));
  }
};

struct upon_error_t {
  template <sender _Sndr, __movable_value _Fn>
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Sndr&& __sndr, _Fn&& __fn) const
      -> __then_sndr<upon_error_t, decay_t<_Fn>, remove_cvref_t<_Sndr>> {
    return execution::__then_make_sndr<upon_error_t>(std::forward<_Sndr>(__sndr), std::forward<_Fn>(__fn));
  }

  template <class _Fn>
    requires constructible_from<decay_t<_Fn>, _Fn>
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Fn&& __fn) const
      noexcept(is_nothrow_constructible_v<decay_t<_Fn>, _Fn>) {
    return execution::__pipeable(std::__bind_back(*this, std::forward<_Fn>(__fn)));
  }
};

struct upon_stopped_t {
  template <sender _Sndr, __movable_value _Fn>
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Sndr&& __sndr, _Fn&& __fn) const
      -> __then_sndr<upon_stopped_t, decay_t<_Fn>, remove_cvref_t<_Sndr>> {
    return execution::__then_make_sndr<upon_stopped_t>(std::forward<_Sndr>(__sndr), std::forward<_Fn>(__fn));
  }

  template <class _Fn>
    requires constructible_from<decay_t<_Fn>, _Fn>
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Fn&& __fn) const
      noexcept(is_nothrow_constructible_v<decay_t<_Fn>, _Fn>) {
    return execution::__pipeable(std::__bind_back(*this, std::forward<_Fn>(__fn)));
  }
};

inline constexpr then_t then{};
inline constexpr upon_error_t upon_error{};
inline constexpr upon_stopped_t upon_stopped{};

} // namespace execution

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___EXECUTION_THEN_H
