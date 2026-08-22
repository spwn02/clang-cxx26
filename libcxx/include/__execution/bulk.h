//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___EXECUTION_BULK_H
#define _LIBCPP___EXECUTION_BULK_H

#include <__concepts/arithmetic.h>
#include <__concepts/constructible.h>
#include <__concepts/same_as.h>
#include <__config>
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
#include <__functional/invoke.h>
#include <__type_traits/conditional.h>
#include <__type_traits/decay.h>
#include <__type_traits/is_execution_policy.h>
#include <__type_traits/is_nothrow_constructible.h>
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

// [exec.bulk]. bulk, bulk_chunked, and bulk_unchunked run a task repeatedly for every index in
// an index space [0, shape). bulk_chunked and bulk_unchunked are the two "real" adaptors here
// (each hand-rolled -- own connect()/get_completion_signatures(), not routed through the
// draft's basic-sender/impls-for/make-sender machinery, per the M3 precedent in
// docs/CXX26_GAPS.md); bulk is a pure call-time composition over bulk_chunked
// ([exec.bulk]p4's `new_f` transform, literally: invoke f once per index by looping inside a
// single bulk_chunked-style chunk).
//
// [exec.bulk]p4's own mechanism for expressing that composition is domain-based
// transform_sender customization (`bulk.transform_sender(set_value, sndr, env)`, dispatched
// via `tag_of_t<Sndr>().transform_sender(...)`) -- exactly the branch
// <__execution/domain.h>'s M2 deviation 4 permanently disables in this fork
// (`default_domain::transform_sender` always takes the "otherwise" static_cast path). Same
// precedent as <__execution/when_all.h>'s `when_all_with_variant`/
// <__execution/stopped_as_error.h>: `bulk_t::operator()` returns `bulk_chunked(sndr, policy,
// shape, new_f)`'s own concrete type directly, rather than producing a distinct
// bulk_t-tagged sender that would rely on a transform_sender indirection this fork never
// fires. Same tag_of_t/sender_for deviation as those files: nothing in scope through M5
// inspects tag_of_t/sender-for on a bulk(...) result.
//
// check-types ([exec.bulk]p6/p8, the Mandates-throwing consteval helper that diagnoses a
// child value datum Func isn't invocable with) is not implemented -- same P3068
// constexpr-exceptions gap as every other adaptor in this sub-plan; a Func that isn't
// invocable with a particular set_value shape's datums simply makes that shape's own
// nothrow-invocability check (used by the completion-signature transform below) evaluate
// `is_nothrow_invocable_v` as false (well-formed either way) rather than reporting a
// dedicated diagnostic -- the completion signature still advertises that set_value shape
// unchanged, plus a spurious set_error_t(exception_ptr), and the real failure only surfaces
// as a hard compile error if that particular shape's receiver is ever actually instantiated.
struct bulk_chunked_t;
struct bulk_unchunked_t;

// [exec.bulk]p3: bulk-algo(sndr, policy, shape, f) stores {policy, shape, f} together as one
// `data` bundle (matching [exec.bulk]p3's own product-type<Policy-or-const-ref, Shape, Func>).
// Policy is stored *by value* only if it models copy_constructible; every concrete policy this
// fork ships (execution::seq/par/par_unseq/unseq, all under _LIBCPP_HAS_EXPERIMENTAL_PSTL in
// <execution>) explicitly deletes its copy constructor, so in practice this always takes the
// const Policy& branch -- storing a reference to the caller's (typically `inline constexpr`,
// static-duration) policy object, matching the standard's own rule literally rather than
// simplifying to "always by reference" as a fork-specific shortcut.
template <class _Policy, class _Shape, class _Func>
struct __bulk_data {
  using __policy_storage_t = __conditional_t<copy_constructible<_Policy>, _Policy, const _Policy&>;

  _LIBCPP_NO_UNIQUE_ADDRESS __policy_storage_t policy;
  _Shape shape;
  _Func f;
};

// Also reused directly by the pipe-form (3-arg) overloads below, as the lambda-captured state:
// a lambda init-capture `[name = expr]` always deduces the captured member's type via `auto`
// (degrading a reference-typed `expr` to a value), so capturing `policy` on its own via
// `[__policy = __bulk_data<...>::__policy_storage_t(...)]` would try to *copy* a non-copyable
// policy into that auto-deduced value member -- a hard compile error, confirmed empirically on
// the first build attempt. Capturing one whole `__bulk_data` object by value instead sidesteps
// this: its `policy` member has an *explicitly declared* type (either Policy or const Policy&,
// never auto-deduced), so copying/moving the enclosing `__bulk_data` struct just copies that
// reference (rebinding to the same static-duration singleton) rather than trying to copy the
// referent itself.

// [exec.bulk]p5/p7's completion-signature transform, generalized over `_Chunked` (the two
// impls-for<bulk_chunked_t>::complete/impls-for<bulk_unchunked_t>::complete lambdas differ
// only in how many times, and with what arguments, f is invoked -- their surrounding
// signature-transform shape is identical): every non-set_value_t signature passes through
// unchanged; a set_value_t(Args...) signature also passes through unchanged (unlike
// <__execution/then.h>'s interception, f's return value is discarded and the *same* Args are
// forwarded onward), but gains an additional set_error_t(exception_ptr) alternative unless
// invoking f is statically known not to throw.
// Whether invoking `f` is statically nothrow, dispatched on `_Chunked` via ordinary partial
// specialization rather than a `_Chunked ? is_nothrow_invocable_v<..., Shape, Shape, ...> :
// is_nothrow_invocable_v<..., Shape, ...>` ternary. A ternary's untaken operand is *not*
// SFINAE-protected the way `if constexpr`'s discarded branch is -- both operands are always
// substituted, and `is_nothrow_invocable_v` for a *generic* lambda (like bulk_t's own `new_f`,
// whose `auto&... vs` parameter can absorb a mismatched arg count without complaint) needs to
// instantiate the lambda's body to deduce its `auto` return type in order to answer the trait
// at all -- body instantiation is not immediate context, so a body that turns out ill-formed
// under the *wrong* (untaken) arity is a hard compile error, not a graceful "not invocable"
// answer. Confirmed empirically: this was a real, reproduced bug during this file's own
// development (`bulk_t`'s pipe-form test hard-errored here on the first attempt), the same
// "immediate context" family of pitfall this sub-plan has hit repeatedly elsewhere (see
// docs/CXX26_GAPS.md's M1/M2 entries) -- but self-inflicted this time (a ternary I wrote),
// not a compiler limitation.
template <bool _Chunked, class _Func, class _Shape, class... _Args>
struct __bulk_nothrow_invocable;
template <class _Func, class _Shape, class... _Args>
struct __bulk_nothrow_invocable<true, _Func, _Shape, _Args...> {
  static constexpr bool value = is_nothrow_invocable_v<_Func&, _Shape, _Shape, _Args&...>;
};
template <class _Func, class _Shape, class... _Args>
struct __bulk_nothrow_invocable<false, _Func, _Shape, _Args...> {
  static constexpr bool value = is_nothrow_invocable_v<_Func&, _Shape, _Args&...>;
};

template <bool _Chunked, class _Func, class _Shape>
struct __bulk_sig_transform {
  template <class _Sig>
  struct __one {
    using type = type_list<_Sig>;
  };

  template <class... _Args>
  struct __one<set_value_t(_Args...)> {
    static constexpr bool __nothrow = __bulk_nothrow_invocable<_Chunked, _Func, _Shape, _Args...>::value;
    using type = __conditional_t<__nothrow, type_list<set_value_t(_Args...)>,
                                  type_list<set_value_t(_Args...), set_error_t(exception_ptr)>>;
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

template <bool _Chunked, class _Func, class _Shape, class _Completions>
using __bulk_signatures_t = typename __bulk_sig_transform<_Chunked, _Func, _Shape>::template __impl<_Completions>::type;

// [exec.bulk]p5/p7's `complete` lambda. On a set_value completion, invokes f -- once with
// (Shape(0), shape, args...) for bulk_chunked (the "invoke exactly one chunk covering the
// whole [0, shape) range" instance the spec's own wording permits, matching what a
// single-threaded fallback with no real parallel-scheduler machinery in scope through M5
// naturally does), or shape times with (i, args...) for i in [0, shape) for bulk_unchunked --
// with the *original* args (by lvalue reference, per [exec.bulk]p9's "args is a pack of
// lvalue subexpressions") forwarded onward to the outer receiver's set_value unchanged
// afterward. Every other completion tag forwards through unchanged. TRY-EVAL semantics: on a
// throwing invocation, completes with set_error(current_exception()) instead.
template <bool _Chunked, class _Policy, class _Shape, class _Func, class _Rcvr>
class __bulk_rcvr {
public:
  using receiver_concept = receiver_tag;

  _LIBCPP_HIDE_FROM_ABI constexpr __bulk_rcvr(__bulk_data<_Policy, _Shape, _Func>&& __data, _Rcvr&& __rcvr)
      : __data_(std::move(__data)), __rcvr_(std::move(__rcvr)) {}

  template <class... _Args>
  _LIBCPP_HIDE_FROM_ABI constexpr void set_value(_Args&&... __args) && noexcept {
    constexpr bool __nothrow = __bulk_nothrow_invocable<_Chunked, _Func, _Shape, _Args...>::value;
    if constexpr (__nothrow) {
      __invoke_and_set_value(std::forward<_Args>(__args)...);
    } else {
      try {
        __invoke_and_set_value(std::forward<_Args>(__args)...);
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
  _LIBCPP_HIDE_FROM_ABI constexpr void __invoke_and_set_value(_Args&&... __args) {
    if constexpr (_Chunked) {
      std::invoke(__data_.f, _Shape(0), __data_.shape, __args...);
    } else {
      for (_Shape __i{}; __i < __data_.shape; ++__i) {
        std::invoke(__data_.f, __i, __args...);
      }
    }
    execution::set_value(std::move(__rcvr_), std::forward<_Args>(__args)...);
  }

  __bulk_data<_Policy, _Shape, _Func> __data_;
  _Rcvr __rcvr_;
};

// An aggregate with public `tag`/`data`/`child` members, matching the (tag, data, ...children)
// shape tag_of_t (<__execution/sender.h>) decomposes via structured bindings -- not routed
// through the draft's generic basic-sender/impls-for machinery: see the M3 entry in
// docs/CXX26_GAPS.md for why that engine isn't buildable on this fork yet. `_Tag` is a
// template parameter (bulk_chunked_t or bulk_unchunked_t), mirroring
// <__execution/then.h>'s `_Tag`-templated `__then_sndr` shape: since `_Tag` is dependent here
// (not a concrete, non-dependent member type the way <__execution/into_variant.h>'s/
// <__execution/when_all.h>'s single-CPO `tag` members are), no forward-declare-then-define-
// out-of-line split is needed -- `_Tag` only needs to be complete once this template is
// actually instantiated, which happens after both bulk_chunked_t and bulk_unchunked_t are
// fully defined below.
template <bool _Chunked, class _Tag, class _Policy, class _Shape, class _Func, class _Sndr>
class __bulk_sndr {
public:
  using sender_concept = sender_tag;

  _LIBCPP_NO_UNIQUE_ADDRESS _Tag tag;
  __bulk_data<_Policy, _Shape, _Func> data;
  _Sndr child;

  template <class _Rcvr>
  _LIBCPP_HIDE_FROM_ABI constexpr auto connect(_Rcvr&& __rcvr) && {
    return execution::connect(std::move(child), __bulk_rcvr<_Chunked, _Policy, _Shape, _Func, remove_cvref_t<_Rcvr>>(
                                                      std::move(data), std::forward<_Rcvr>(__rcvr)));
  }

  // [exec.adapt.general]p3.2: a parent sender with a single child sndr has an associated
  // attribute object equal to FWD-ENV(get_env(sndr)).
  _LIBCPP_HIDE_FROM_ABI constexpr auto get_env() const noexcept {
    return execution::__fwd_env_fn(execution::get_env(child));
  }

  template <class _Self, class _Env>
    requires sender_in<_Sndr, __fwd_env<remove_cvref_t<_Env>>>
  _LIBCPP_HIDE_FROM_ABI static consteval auto get_completion_signatures() {
    using __child_sigs = completion_signatures_of_t<_Sndr, __fwd_env<remove_cvref_t<_Env>>>;
    return __bulk_signatures_t<_Chunked, _Func, _Shape, __child_sigs>{};
  }
};

template <bool _Chunked, class _Tag, class _Sndr, class _Policy, class _Shape, class _Func>
_LIBCPP_HIDE_FROM_ABI constexpr auto __bulk_make_sndr(_Sndr&& __sndr, _Policy&& __policy, _Shape __shape, _Func&& __f) {
  using __policy_t = remove_cvref_t<_Policy>;
  using __data_t   = __bulk_data<__policy_t, _Shape, decay_t<_Func>>;
  return __bulk_sndr<_Chunked, _Tag, __policy_t, _Shape, decay_t<_Func>, remove_cvref_t<_Sndr>>{
      {},
      __data_t{std::forward<_Policy>(__policy), __shape, decay_t<_Func>(std::forward<_Func>(__f))},
      std::forward<_Sndr>(__sndr)};
}

struct bulk_chunked_t : sender_adaptor_closure<bulk_chunked_t> {
  template <sender _Sndr, class _Policy, integral _Shape, class _Func>
    requires is_execution_policy_v<remove_cvref_t<_Policy>> && copy_constructible<decay_t<_Func>>
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Sndr&& __sndr, _Policy&& __policy, _Shape __shape, _Func&& __f) const {
    return execution::__bulk_make_sndr<true, bulk_chunked_t>(
        std::forward<_Sndr>(__sndr), std::forward<_Policy>(__policy), __shape, std::forward<_Func>(__f));
  }

  template <class _Policy, integral _Shape, class _Func>
    requires is_execution_policy_v<remove_cvref_t<_Policy>> && copy_constructible<decay_t<_Func>>
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Policy&& __policy, _Shape __shape, _Func&& __f) const {
    using __data_t = __bulk_data<remove_cvref_t<_Policy>, _Shape, decay_t<_Func>>;
    return execution::__pipeable(
        [__data = __data_t{std::forward<_Policy>(__policy), __shape, decay_t<_Func>(std::forward<_Func>(__f))}](
            auto&& __sndr) mutable {
          return bulk_chunked_t{}(std::forward<decltype(__sndr)>(__sndr), __data.policy, __data.shape, std::move(__data.f));
        });
  }
};

struct bulk_unchunked_t : sender_adaptor_closure<bulk_unchunked_t> {
  template <sender _Sndr, class _Policy, integral _Shape, class _Func>
    requires is_execution_policy_v<remove_cvref_t<_Policy>> && copy_constructible<decay_t<_Func>>
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Sndr&& __sndr, _Policy&& __policy, _Shape __shape, _Func&& __f) const {
    return execution::__bulk_make_sndr<false, bulk_unchunked_t>(
        std::forward<_Sndr>(__sndr), std::forward<_Policy>(__policy), __shape, std::forward<_Func>(__f));
  }

  template <class _Policy, integral _Shape, class _Func>
    requires is_execution_policy_v<remove_cvref_t<_Policy>> && copy_constructible<decay_t<_Func>>
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Policy&& __policy, _Shape __shape, _Func&& __f) const {
    using __data_t = __bulk_data<remove_cvref_t<_Policy>, _Shape, decay_t<_Func>>;
    return execution::__pipeable(
        [__data = __data_t{std::forward<_Policy>(__policy), __shape, decay_t<_Func>(std::forward<_Func>(__f))}](
            auto&& __sndr) mutable {
          return bulk_unchunked_t{}(std::forward<decltype(__sndr)>(__sndr), __data.policy, __data.shape, std::move(__data.f));
        });
  }
};

inline constexpr bulk_chunked_t bulk_chunked{};
inline constexpr bulk_unchunked_t bulk_unchunked{};

// [exec.bulk]p4: bulk(sndr, policy, shape, f) is expression-equivalent (on this fork, per the
// file-level deviation note above) to
// `bulk_chunked(sndr, policy, shape, new_f)`, where `new_f(begin, end, vs...)` invokes
// `f(i, vs...)` for every `i` in `[begin, end)` -- reproducing bulk's own "invoke f(i,
// args...) for every i from 0 to shape" semantics through bulk_chunked's single-chunk-per-call
// default behavior (see the `__bulk_rcvr` comment above): since bulk_chunked here always
// invokes its own Func exactly once with the *whole* [0, shape) range, `new_f`'s internal loop
// ends up covering every index exactly once, matching bulk's contract precisely.
struct bulk_t : sender_adaptor_closure<bulk_t> {
  template <sender _Sndr, class _Policy, integral _Shape, class _Func>
    requires is_execution_policy_v<remove_cvref_t<_Policy>> && copy_constructible<decay_t<_Func>>
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Sndr&& __sndr, _Policy&& __policy, _Shape __shape, _Func&& __f) const {
    return execution::bulk_chunked(
        std::forward<_Sndr>(__sndr), std::forward<_Policy>(__policy), __shape,
        [__func = decay_t<_Func>(std::forward<_Func>(__f))](_Shape __begin, _Shape __end, auto&... __vs) mutable
            noexcept(is_nothrow_invocable_v<decay_t<_Func>&, _Shape, decltype(__vs)...>) {
          while (__begin != __end) {
            __func(__begin++, __vs...);
          }
        });
  }

  template <class _Policy, integral _Shape, class _Func>
    requires is_execution_policy_v<remove_cvref_t<_Policy>> && copy_constructible<decay_t<_Func>>
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Policy&& __policy, _Shape __shape, _Func&& __f) const {
    using __data_t = __bulk_data<remove_cvref_t<_Policy>, _Shape, decay_t<_Func>>;
    return execution::__pipeable(
        [__data = __data_t{std::forward<_Policy>(__policy), __shape, decay_t<_Func>(std::forward<_Func>(__f))}](
            auto&& __sndr) mutable {
          return bulk_t{}(std::forward<decltype(__sndr)>(__sndr), __data.policy, __data.shape, std::move(__data.f));
        });
  }
};

inline constexpr bulk_t bulk{};

} // namespace execution

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___EXECUTION_BULK_H
