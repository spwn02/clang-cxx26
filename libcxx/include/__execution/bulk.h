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
#include <__execution/completion_functions.h>
#include <__execution/completion_attrs.h>
#include <__execution/completion_signatures.h>
#include <__execution/connect.h>
#include <__execution/env.h>
#include <__execution/fwd_env.h>
#include <__execution/get_completion_signatures.h>
#include <__execution/get_env.h>
#include <__execution/get_scheduler.h>
#include <__execution/movable_value.h>
#include <__execution/operation_state.h>
#include <__execution/parallel_scheduler.h>
#include <__execution/policies.h>
#include <__execution/system_context_replaceability.h>
#include <__execution/task_scheduler.h>
#include <__execution/receiver.h>
#include <__execution/sender.h>
#include <__execution/sender_adaptor_closure.h>
#include <__functional/bind_back.h>
#include <__functional/invoke.h>
#include <__mutex/lock_guard.h>
#include <__mutex/mutex.h>
#include <__type_traits/conditional.h>
#include <__type_traits/decay.h>
#include <__type_traits/is_execution_policy.h>
#include <__type_traits/is_nothrow_constructible.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/forward.h>
#include <__utility/forward_like.h>
#include <__utility/move.h>
#include <algorithm>
#include <atomic>
#include <cstddef>
#include <exception>
#include <thread>
#include <tuple>
#include <vector>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_PUSH_MACROS
#include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

namespace execution {

// [exec.bulk]. bulk, bulk_chunked, and bulk_unchunked run a task repeatedly for every index in
// an index space [0, shape). bulk_chunked and bulk_unchunked are the two "real" adaptors here
// (each with its own connect()/get_completion_signatures()); bulk is a make-sender-shaped sender
// of its own tag (tag/data/child), lowered by
// `bulk.transform_sender(set_value, sndr, env)` ([exec.bulk]p4's `new_f` transform, literally: invoke f
// once per index by looping inside a single bulk_chunked chunk) when it is connected, through
// default_domain::transform_sender (`tag_of_t<Sndr>().transform_sender(...)`).
//
// check-types ([exec.bulk]p6/p8) is a constraint on get_completion_signatures: a Func that isn't
// invocable (as an lvalue) with every set_value shape of the child makes the member not viable, so the
// generic [exec.getcomplsigs] fallback throws and sender_in is false; a child that never completes with
// set_value is unconstrained.
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

// The closure object returned by the partial application bulk-algo(policy, shape, f): a perfect forwarding call wrapper
// ([exec.adapt.obj]) over the policy, shape and function, which stay valid when the closure is called more than once:
// an lvalue closure passes them as lvalues (the function is copied), an rvalue one may move the function.
template <class _Algo, class _Data>
struct __bulk_closure {
  _Data __data;

  template <class _Sndr>
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Sndr&& __sndr) const& {
    return _Algo{}(std::forward<_Sndr>(__sndr), __data.policy, __data.shape, __data.f);
  }
  template <class _Sndr>
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Sndr&& __sndr) && {
    return _Algo{}(std::forward<_Sndr>(__sndr), __data.policy, __data.shape, std::move(__data.f));
  }
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
// "immediate context" family of pitfall: the original ternary made invalid expressions hard
// errors instead of substitution failures.
template <bool _Chunked, class _Func, class _Shape, class... _Args>
struct __bulk_nothrow_invocable;

template <bool _Chunked, class _Func, class _Shape, class _List>
struct __bulk_invocable_one;
template <class _Func, class _Shape, class... _Args>
struct __bulk_invocable_one<true, _Func, _Shape, __exec_type_list<_Args...>> {
  static constexpr bool value = invocable<_Func&, _Shape, _Shape, _Args&...>;
};
template <class _Func, class _Shape, class... _Args>
struct __bulk_invocable_one<false, _Func, _Shape, __exec_type_list<_Args...>> {
  static constexpr bool value = invocable<_Func&, _Shape, _Args&...>;
};
template <class _Func, class _Shape, class... _Args>
struct __bulk_nothrow_invocable<true, _Func, _Shape, _Args...> {
  static constexpr bool value = is_nothrow_invocable_v<_Func&, _Shape, _Shape, _Args&...>;
};
template <class _Func, class _Shape, class... _Args>
struct __bulk_nothrow_invocable<false, _Func, _Shape, _Args...> {
  static constexpr bool value = is_nothrow_invocable_v<_Func&, _Shape, _Args&...>;
};

// check-types helper: whether `_Func&` is invocable for every value completion (`_Lists`: __exec_type_list of the datum lists).
template <bool _Chunked, class _Func, class _Shape, class _Lists>
inline constexpr bool __bulk_invocable_v = false;
template <class _Func, class _Shape, class... _Lists>
inline constexpr bool __bulk_invocable_v<true, _Func, _Shape, __exec_type_list<_Lists...>> =
    (__bulk_invocable_one<true, _Func, _Shape, _Lists>::value && ...);
template <class _Func, class _Shape, class... _Lists>
inline constexpr bool __bulk_invocable_v<false, _Func, _Shape, __exec_type_list<_Lists...>> =
    (__bulk_invocable_one<false, _Func, _Shape, _Lists>::value && ...);

// The contributors of bulk/bulk_chunked/bulk_unchunked: the function is invoked where the child completed with
// set_value and its exception, if it can throw, is an error completion in the same place; all completions of the child
// are otherwise forwarded.
template <bool _Chunked, class _Func, class _Shape>
struct __bulk_contrib {
  template <class _Lists>
  struct __throws;
  template <class... _Lists>
  struct __throws<__exec_type_list<_Lists...>> {
    template <class _List>
    struct __one;
    template <class... _Args>
    struct __one<__exec_type_list<_Args...>> {
      static constexpr bool value = !__bulk_nothrow_invocable<_Chunked, _Func, _Shape, _Args...>::value;
    };
    static constexpr bool value = (__one<_Lists>::value || ... || false);
  };

  template <class _ChildSigs, class _Out>
  static consteval unsigned __mask() {
    constexpr bool __may_throw = __throws<__gather_signatures<set_value_t, _ChildSigs, __exec_type_list, __exec_type_list>>::value;
    return __intercept_contributors<set_value_t, set_value_t, __may_throw, _ChildSigs>::template __mask<_Out>();
  }
};

template <bool _Chunked, class _Func, class _Shape>
struct __bulk_sig_transform {
  template <class _Sig>
  struct __one {
    using type = __exec_type_list<_Sig>;
  };

  template <class... _Args>
  struct __one<set_value_t(_Args...)> {
    static constexpr bool __nothrow = __bulk_nothrow_invocable<_Chunked, _Func, _Shape, _Args...>::value;
    using type = __conditional_t<__nothrow, __exec_type_list<set_value_t(_Args...)>,
                                  __exec_type_list<set_value_t(_Args...), set_error_t(exception_ptr)>>;
  };

  template <class _List>
  struct __dedup;
  template <class... _Ts>
  struct __dedup<__exec_type_list<_Ts...>> {
    using type = __dedup_type_list_t<_Ts...>;
  };

  template <class _List>
  struct __to_completion_signatures;
  template <class... _Sigs>
  struct __to_completion_signatures<__exec_type_list<_Sigs...>> {
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
// single-threaded fallback naturally does), or shape times with (i, args...) for i in [0, shape)
// for bulk_unchunked --
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

#if _LIBCPP_HAS_THREADS
// [exec.par.scheduler]: bulk work for a parallel scheduler is handed to its bound
// replaceable backend. The backend chooses the ranges and invokes the proxy's
// execute() for each range; the proxy preserves the predecessor's values until
// the backend completes the operation.
// The backend owns partitioning and calls execute for each range. It may call
// execute concurrently; completion is delivered once after all calls return.
template <bool _Chunked, class _Shape, class _Func, class _Rcvr, class... _Args>
class __bulk_backend_job final : public parallel_scheduler_replacement::bulk_item_receiver_proxy {
  using _StopToken = decltype(std::get_stop_token(execution::get_env(std::declval<_Rcvr&>())));

public:
  template <class... _UArgs>
  _LIBCPP_HIDE_FROM_ABI __bulk_backend_job(
      _Func&& __f, _Shape __shape, _Rcvr&& __rcvr,
      shared_ptr<parallel_scheduler_replacement::parallel_scheduler_backend> __backend, _UArgs&&... __args)
      : __f_(std::move(__f)), __args_(std::forward<_UArgs>(__args)...), __rcvr_(std::move(__rcvr)),
        __backend_(std::move(__backend)), __shape_(__shape) {}

  _LIBCPP_HIDE_FROM_ABI void __start() noexcept {
    if constexpr (_Chunked)
      __backend_->schedule_bulk_chunked(static_cast<size_t>(__shape_), *this,
                                        span<byte>(__backend_storage_, sizeof(__backend_storage_)));
    else
      __backend_->schedule_bulk_unchunked(static_cast<size_t>(__shape_), *this,
                                          span<byte>(__backend_storage_, sizeof(__backend_storage_)));
  }

private:
  _LIBCPP_HIDE_FROM_ABI void execute(size_t __begin, size_t __end) noexcept override {
    try {
      if constexpr (_Chunked) {
        std::apply([&](_Args&... __a) {
          std::invoke(__f_, static_cast<_Shape>(__begin), static_cast<_Shape>(__end), __a...);
        }, __args_);
      } else {
        std::apply([&](_Args&... __a) {
          for (size_t __i = __begin; __i != __end; ++__i)
            std::invoke(__f_, static_cast<_Shape>(__i), __a...);
        }, __args_);
      }
    } catch (...) {
      lock_guard<mutex> __lock(__err_mtx_);
      if (!__first_err_)
        __first_err_ = std::current_exception();
    }
  }

  _LIBCPP_HIDE_FROM_ABI void set_value() noexcept override {
    if (__first_err_)
      execution::set_error(std::move(__rcvr_), std::move(__first_err_));
    else
      std::apply([&](_Args&... __a) { execution::set_value(std::move(__rcvr_), std::move(__a)...); }, __args_);
    delete this;
  }

  _LIBCPP_HIDE_FROM_ABI void set_error(exception_ptr __err) noexcept override {
    execution::set_error(std::move(__rcvr_), std::move(__err));
    delete this;
  }

  _LIBCPP_HIDE_FROM_ABI void set_stopped() noexcept override {
    execution::set_stopped(std::move(__rcvr_));
    delete this;
  }

  _LIBCPP_HIDE_FROM_ABI bool
  __query_env(const type_info& __query_type, const type_info& __result_type, const void*, void* __result_storage)
      const noexcept override {
    if (__query_type == typeid(get_stop_token_t) && __result_type == typeid(_StopToken)) {
      ::new (__result_storage) _StopToken(std::get_stop_token(execution::get_env(__rcvr_)));
      return true;
    }
    return false;
  }

  _Func __f_;
  tuple<_Args...> __args_;
  _Rcvr __rcvr_;
  shared_ptr<parallel_scheduler_replacement::parallel_scheduler_backend> __backend_;
  _Shape __shape_;
  mutex __err_mtx_;
  exception_ptr __first_err_;
  alignas(max_align_t) byte __backend_storage_[64];
};

template <bool _Chunked, class _Policy, class _Shape, class _Func, class _Rcvr>
class __bulk_backend_rcvr {
public:
  using receiver_concept = receiver_tag;

  _LIBCPP_HIDE_FROM_ABI __bulk_backend_rcvr(
      __bulk_data<_Policy, _Shape, _Func>&& __data, _Rcvr&& __rcvr,
      shared_ptr<parallel_scheduler_replacement::parallel_scheduler_backend> __backend)
      : __data_(std::move(__data)), __rcvr_(std::move(__rcvr)), __backend_(std::move(__backend)) {}

  template <class... _Args>
  _LIBCPP_HIDE_FROM_ABI void set_value(_Args&&... __args) && noexcept {
    using __job_t = __bulk_backend_job<_Chunked, _Shape, _Func, _Rcvr, decay_t<_Args>...>;
    auto* __job = new __job_t(std::move(__data_.f), __data_.shape, std::move(__rcvr_),
                              std::move(__backend_), std::forward<_Args>(__args)...);
    __job->__start();
  }

  template <class _Err>
  _LIBCPP_HIDE_FROM_ABI void set_error(_Err&& __err) && noexcept {
    execution::set_error(std::move(__rcvr_), std::forward<_Err>(__err));
  }

  _LIBCPP_HIDE_FROM_ABI void set_stopped() && noexcept { execution::set_stopped(std::move(__rcvr_)); }
  _LIBCPP_HIDE_FROM_ABI auto get_env() const noexcept {
    return execution::__fwd_env_fn(execution::get_env(__rcvr_));
  }

private:
  __bulk_data<_Policy, _Shape, _Func> __data_;
  _Rcvr __rcvr_;
  shared_ptr<parallel_scheduler_replacement::parallel_scheduler_backend> __backend_;
};
#endif // _LIBCPP_HAS_THREADS

// An aggregate with public `tag`/`data`/`child` members, matching the (tag, data, ...children)
// shape tag_of_t (<__execution/sender.h>) decomposes via structured bindings. `_Tag` is a
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
  _LIBCPP_HIDE_FROM_ABI constexpr auto connect(_Rcvr&& __rcvr) && noexcept(
      noexcept(__connect_with(std::move(*this), std::declval<_Rcvr>()))) {
    return __connect_with(std::move(*this), std::forward<_Rcvr>(__rcvr));
  }

  template <class _Rcvr>
    requires copy_constructible<__bulk_data<_Policy, _Shape, _Func>> && copy_constructible<_Sndr>
  _LIBCPP_HIDE_FROM_ABI constexpr auto connect(_Rcvr&& __rcvr) const& noexcept(
      noexcept(__connect_with(*this, std::declval<_Rcvr>()))) {
    return __connect_with(*this, std::forward<_Rcvr>(__rcvr));
  }

private:
  // (potentially throwing, as the receiver of the parallel scheduler path allocates)
  template <class _Self, class _Rcvr>
  _LIBCPP_HIDE_FROM_ABI static constexpr auto __connect_with(_Self&& __self, _Rcvr&& __rcvr) {
    using __data_t = __bulk_data<_Policy, _Shape, _Func>;
#if _LIBCPP_HAS_THREADS
    // The parallel scheduler completion domain selects the bulk customization.
    // Query the child's completion scheduler to retain the backend bound to it.
    if constexpr (requires {
                    execution::__try_query(execution::get_env(__self.child), get_completion_scheduler<set_value_t>);
                  }) {
      using __child_sch_t =
          remove_cvref_t<decltype(execution::__try_query(execution::get_env(__self.child), get_completion_scheduler<set_value_t>))>;
      if constexpr (same_as<__child_sch_t, parallel_scheduler> || same_as<__child_sch_t, task_scheduler>) {
        return execution::connect(
            std::forward_like<_Self>(__self.child), __bulk_backend_rcvr<_Chunked, _Policy, _Shape, _Func, remove_cvref_t<_Rcvr>>(
                                   __data_t(std::forward_like<_Self>(__self.data)), std::forward<_Rcvr>(__rcvr),
                                   execution::__try_query(execution::get_env(__self.child), get_completion_scheduler<set_value_t>).__get_backend()));
      } else {
        return execution::connect(std::forward_like<_Self>(__self.child),
                                   __bulk_rcvr<_Chunked, _Policy, _Shape, _Func, remove_cvref_t<_Rcvr>>(
                                       __data_t(std::forward_like<_Self>(__self.data)), std::forward<_Rcvr>(__rcvr)));
      }
    } else
#endif // _LIBCPP_HAS_THREADS
    {
      return execution::connect(std::forward_like<_Self>(__self.child), __bulk_rcvr<_Chunked, _Policy, _Shape, _Func, remove_cvref_t<_Rcvr>>(
                                                        __data_t(std::forward_like<_Self>(__self.data)), std::forward<_Rcvr>(__rcvr)));
    }
  }

public:
  // [exec.adapt.general]p3.2 and [exec.snd.general]: the attributes of the child, but for the completion queries.
  _LIBCPP_HIDE_FROM_ABI constexpr auto get_env() const noexcept {
    using __child_attrs_t = remove_cvref_t<decltype(execution::get_env(child))>;
    return __completion_attrs<__bulk_contrib<_Chunked, _Func, _Shape>, _Sndr, __child_attrs_t>(execution::get_env(child));
  }

  // check-types ([exec.bulk]p6, p8): the function is invocable with (shape, args...) or (begin, end, args...) for the
  // result datums (lvalues) of every value completion.
  template <class _Self, class... _Env>
    requires sender_in<_Sndr, __fwd_env_of_first_t<_Env...>> &&
             __bulk_invocable_v<_Chunked, _Func, _Shape,
                                __gather_signatures<set_value_t,
                                                    completion_signatures_of_t<_Sndr, __fwd_env_of_first_t<_Env...>>,
                                                    __exec_type_list,
                                                    __exec_type_list>>
  _LIBCPP_HIDE_FROM_ABI static consteval auto get_completion_signatures() {
    using __child_sigs = completion_signatures_of_t<_Sndr, __fwd_env_of_first_t<_Env...>>;
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

struct bulk_chunked_t {
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
    return execution::__pipeable(__bulk_closure<bulk_chunked_t, __data_t>{
        __data_t{std::forward<_Policy>(__policy), __shape, decay_t<_Func>(std::forward<_Func>(__f))}});
  }
};

struct bulk_unchunked_t {
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
    return execution::__pipeable(__bulk_closure<bulk_unchunked_t, __data_t>{
        __data_t{std::forward<_Policy>(__policy), __shape, decay_t<_Func>(std::forward<_Func>(__f))}});
  }
};

inline constexpr bulk_chunked_t bulk_chunked{};
inline constexpr bulk_unchunked_t bulk_unchunked{};

// [exec.bulk]p4: bulk(sndr, policy, shape, f) is make-sender(bulk, product-type{policy, shape, f}, sndr), an aggregate
// with public `tag`/`data`/`child` members. When it is connected to a receiver whose domain does not customize bulk
// it is lowered by bulk.transform_sender(set_value, sndr, env) to
// `bulk_chunked(child, policy, shape, new_f)`, where `new_f(begin, end, vs...)` invokes `f(i, vs...)` for every `i` in
// `[begin, end)`.
struct bulk_t;

template <class _Policy, class _Shape, class _Func, class _Sndr>
class __bulk_front_sndr;

// [exec.bulk]p5's new_f: invokes f(i, vs...) for every i in [begin, end). A class rather than a lambda: it is constrained on
// f being invocable, which must be a soft failure for check-types (a lambda with such a constraint trips an assertion of
// this compiler when it is created in a function template that is instantiated during overload resolution).
template <class _Func, class _Shape>
struct __bulk_chunk_fn {
  _Func __func_;

  template <class... _Vs>
    requires invocable<_Func&, _Shape, _Vs&...>
  _LIBCPP_HIDE_FROM_ABI constexpr void operator()(_Shape __begin, _Shape __end, _Vs&... __vs) noexcept(
      is_nothrow_invocable_v<_Func&, _Shape, _Vs&...>) {
    while (__begin != __end) {
      __func_(__begin++, __vs...);
    }
  }
};

struct bulk_t {
  template <sender _Sndr, class _Policy, integral _Shape, class _Func>
    requires is_execution_policy_v<remove_cvref_t<_Policy>> && copy_constructible<decay_t<_Func>>
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Sndr&& __sndr, _Policy&& __policy, _Shape __shape, _Func&& __f) const;

  template <class _Policy, integral _Shape, class _Func>
    requires is_execution_policy_v<remove_cvref_t<_Policy>> && copy_constructible<decay_t<_Func>>
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Policy&& __policy, _Shape __shape, _Func&& __f) const {
    using __data_t = __bulk_data<remove_cvref_t<_Policy>, _Shape, decay_t<_Func>>;
    return execution::__pipeable(__bulk_closure<bulk_t, __data_t>{
        __data_t{std::forward<_Policy>(__policy), __shape, decay_t<_Func>(std::forward<_Func>(__f))}});
  }

  // [exec.bulk]p5: bulk.transform_sender(set_value, sndr, env), for a sender of this tag.
  template <class _Sndr, class _Env>
    requires __sender_for<_Sndr, bulk_t>
  _LIBCPP_HIDE_FROM_ABI static constexpr auto transform_sender(set_value_t, _Sndr&& __sndr, const _Env&) {
    // (no structured binding here: a constrained lambda that captures one trips an assertion of the compiler)
    auto&& __data  = __sndr.data;
    auto&& __child = __sndr.child;
    using _Shape   = remove_cvref_t<decltype(__data.shape)>;
    using _Func  = remove_cvref_t<decltype(__data.f)>;
    return execution::bulk_chunked(
        std::forward_like<_Sndr>(__child), __data.policy, __data.shape,
        __bulk_chunk_fn<_Func, _Shape>{std::forward_like<_Sndr>(__data.f)});
  }
};

template <class _Policy, class _Shape, class _Func, class _Sndr>
class __bulk_front_sndr {
public:
  using sender_concept = sender_tag;

  _LIBCPP_NO_UNIQUE_ADDRESS bulk_t tag;
  __bulk_data<_Policy, _Shape, _Func> data;
  _Sndr child;

  _LIBCPP_HIDE_FROM_ABI constexpr auto get_env() const noexcept {
    using __child_attrs_t = remove_cvref_t<decltype(execution::get_env(child))>;
    return __completion_attrs<__bulk_contrib<false, _Func, _Shape>, _Sndr, __child_attrs_t>(execution::get_env(child));
  }

  // Only reached for a sender that was not transformed, which cannot happen: the transformation has no constraints
  // but the shape of the sender.
  template <class _Self, class... _Env>
  _LIBCPP_HIDE_FROM_ABI static consteval auto get_completion_signatures() {
    if constexpr (sizeof...(_Env) == 0) {
      return execution::__lowered_signatures_without_env<_Self>();
    } else {
      throw __unspecified_exception();
      return completion_signatures<>();
    }
  }
};

template <sender _Sndr, class _Policy, integral _Shape, class _Func>
  requires is_execution_policy_v<remove_cvref_t<_Policy>> && copy_constructible<decay_t<_Func>>
_LIBCPP_HIDE_FROM_ABI constexpr auto bulk_t::operator()(_Sndr&& __sndr, _Policy&& __policy, _Shape __shape, _Func&& __f) const {
  using __policy_t = remove_cvref_t<_Policy>;
  using __data_t   = __bulk_data<__policy_t, _Shape, decay_t<_Func>>;
  return __bulk_front_sndr<__policy_t, _Shape, decay_t<_Func>, remove_cvref_t<_Sndr>>{
      {},
      __data_t{std::forward<_Policy>(__policy), __shape, decay_t<_Func>(std::forward<_Func>(__f))},
      std::forward<_Sndr>(__sndr)};
}

inline constexpr bulk_t bulk{};

#if _LIBCPP_HAS_THREADS
// The task scheduler's erased backend starts a sender formed with the wrapped
// scheduler. This lets its domain select a bulk implementation before erasure.
template <class _Sch>
class __task_scheduler_backend final : public parallel_scheduler_replacement::parallel_scheduler_backend {
  _Sch __sch_;

  struct __just_sender {
    using sender_concept = sender_tag;
    struct __attrs {
      _Sch __sch;
      _LIBCPP_HIDE_FROM_ABI _Sch query(get_completion_scheduler_t<set_value_t>) const noexcept { return __sch; }
      _LIBCPP_HIDE_FROM_ABI auto query(get_completion_domain_t<set_value_t>) const noexcept
        requires requires(const _Sch& __s) { __s.query(get_completion_domain<set_value_t>); }
      { return __sch.query(get_completion_domain<set_value_t>); }
    };
    const _Sch* __sch;
    _LIBCPP_HIDE_FROM_ABI __attrs get_env() const noexcept { return {*__sch}; }
    template <class _Rcvr>
    struct __op {
      using operation_state_concept = operation_state_tag;
      _Rcvr __rcvr;
      _LIBCPP_HIDE_FROM_ABI void start() & noexcept { execution::set_value(std::move(__rcvr)); }
    };
    template <class _Rcvr>
    _LIBCPP_HIDE_FROM_ABI auto connect(_Rcvr&& __r) const {
      return __op<remove_cvref_t<_Rcvr>>{std::forward<_Rcvr>(__r)};
    }
    template <class, class...>
    _LIBCPP_HIDE_FROM_ABI static consteval auto get_completion_signatures() {
      return completion_signatures<set_value_t()>{};
    }
  };

  template <class _Sndr>
  struct __state final : __task_backend_completion {
    using __op_t = connect_result_t<_Sndr, __task_backend_receiver>;
    __op_t __op_;
    parallel_scheduler_replacement::receiver_proxy* __rcvr_;

    _LIBCPP_HIDE_FROM_ABI __state(_Sndr&& __sndr, parallel_scheduler_replacement::receiver_proxy& __r)
        : __op_(execution::connect(std::move(__sndr), __task_backend_receiver{this})), __rcvr_(&__r) {}
    _LIBCPP_HIDE_FROM_ABI void __value() noexcept override {
      __rcvr_->set_value();
      delete this;
    }
    _LIBCPP_HIDE_FROM_ABI void __error(exception_ptr __err) noexcept override {
      __rcvr_->set_error(std::move(__err));
      delete this;
    }
    _LIBCPP_HIDE_FROM_ABI void __stopped() noexcept override {
      __rcvr_->set_stopped();
      delete this;
    }
  };

  template <class _Sndr>
  _LIBCPP_HIDE_FROM_ABI static void __launch(_Sndr&& __sndr, parallel_scheduler_replacement::receiver_proxy& __r) noexcept {
    auto* __st = new __state<remove_cvref_t<_Sndr>>(std::forward<_Sndr>(__sndr), __r);
    execution::start(__st->__op_);
  }

public:
  _LIBCPP_HIDE_FROM_ABI explicit __task_scheduler_backend(_Sch __sch) : __sch_(std::move(__sch)) {}
  _LIBCPP_HIDE_FROM_ABI void schedule(parallel_scheduler_replacement::receiver_proxy& __r, span<byte>) noexcept override {
    __launch(execution::schedule(__sch_), __r);
  }
  _LIBCPP_HIDE_FROM_ABI void schedule_bulk_chunked(
      size_t __shape, parallel_scheduler_replacement::bulk_item_receiver_proxy& __r, span<byte>) noexcept override {
    auto __fn = [&__r](size_t __begin, size_t __end) { __r.execute(__begin, __end); };
    __launch(execution::transform_sender(
                 execution::bulk_chunked(__just_sender{&__sch_}, execution::par, __shape, __fn), env<>{}), __r);
  }
  _LIBCPP_HIDE_FROM_ABI void schedule_bulk_unchunked(
      size_t __shape, parallel_scheduler_replacement::bulk_item_receiver_proxy& __r, span<byte>) noexcept override {
    auto __fn = [&__r](size_t __i) { __r.execute(__i, __i + 1); };
    __launch(execution::transform_sender(
                 execution::bulk_unchunked(__just_sender{&__sch_}, execution::par, __shape, __fn), env<>{}), __r);
  }
};
#endif

} // namespace execution

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP___EXECUTION_BULK_H
