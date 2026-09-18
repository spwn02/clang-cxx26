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
#include <__execution/env.h>
#include <__execution/fwd_env.h>
#include <__execution/get_completion_signatures.h>
#include <__execution/get_env.h>
#include <__execution/get_scheduler.h>
#include <__execution/movable_value.h>
#include <__execution/operation_state.h>
#include <__execution/parallel_scheduler.h>
#include <__execution/receiver.h>
#include <__execution/sender.h>
#include <__execution/sender_adaptor_closure.h>
#include <__functional/invoke.h>
#include <__mutex/lock_guard.h>
#include <__mutex/mutex.h>
#include <__type_traits/conditional.h>
#include <__type_traits/decay.h>
#include <__type_traits/is_execution_policy.h>
#include <__type_traits/is_nothrow_constructible.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/forward.h>
#include <__utility/move.h>
#include <algorithm>
#include <atomic>
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

#if _LIBCPP_HAS_THREADS
// Pass 2 (P2079R10, see docs/design/parallel_scheduler_p2079.md): parallel_scheduler's own
// bulk_chunked customization, later widened in Pass 3 to bulk_unchunked_t too (both share this
// same job/receiver machinery, parameterized on _Chunked exactly the way __bulk_rcvr's own
// single-threaded fallback already is above). [exec.bulk]p4's specified mechanism for this kind
// of customization (domain-based transform_sender, see the file-level comment above) is
// permanently disabled on this fork; this is the documented, pragmatic replacement -- probing
// the input sender's completion scheduler directly inside __bulk_sndr::connect() (below) and,
// when it is parallel_scheduler, actually splitting [0, shape) across the pool's worker
// threads instead of invoking f once inline. Chunked and unchunked differ only in what each
// worker does with its own [begin, end) sub-range once dispatched: one call to f covering the
// whole sub-range for bulk_chunked_t (matching what a single implementation-chosen chunk may
// do), versus one call to f per index within it for bulk_unchunked_t (matching
// [exec.bulk]p9's "f is invoked once for every index" requirement) -- see __invoke_chunk below.
//
// A chunk's own completion always runs ON a pool worker thread -- so completing the whole
// bulk operation cannot block waiting for the other chunks (this fork's Pass-1 pool has no
// work-stealing/helping by design, so a blocking wait here would consume a worker and can
// deadlock the whole pool the moment concurrent bulk operations reach or exceed the worker
// count; confirmed via advisor review before writing this, and verified empirically -- see
// exec.parallel.scheduler's bulk test -- by forcing a single-worker pool).
//
// The probe below only ever sees a parallel_scheduler for a *direct*
// `schedule(get_parallel_scheduler()) | bulk_chunked(...)` chain: FWD-ENV
// (<__execution/fwd_env.h>), the same shared utility every adaptor in this fork uses for its
// own get_env() (including __bulk_sndr's own, just below, and then_sndr's), does not forward
// get_completion_scheduler through -- confirmed empirically (schedule(sch) | then(f) |
// bulk_chunked(...) falls back to the sequential single-chunk path, silently and correctly,
// not an error). This matches every other adaptor's existing behavior in this fork; it isn't a
// new gap this file introduces, but it does mean the customization's practical reach is
// narrower than "wherever a parallel_scheduler's completion is reachable" -- only the direct
// chain, today. Widening this (e.g. making bulk_chunked's own get_env() re-expose its child's
// completion scheduler) is a follow-up, not attempted here. Instead, each chunk
// decrements a shared atomic counter on completion; the chunk that takes it to zero completes
// the outer receiver. fetch_sub uses acq_rel (not relaxed) so the completing chunk's read of
// the shared error/args state actually observes every other chunk's writes to it.
//
// args must outlive the initial set_value() call (chunks complete asynchronously, well after
// that call returns), so they are decay-copied once into __bulk_parallel_job below -- matching
// how every other cross-thread-continuation adaptor in this fork (e.g.
// <__execution/async_scope.h>'s __spawn_op) materializes storage for anything that must
// survive past its synchronous call, rather than relying on the child sender's own temporaries.
// bulk_chunked_t's own `copy_constructible<decay_t<_Func>>` constraint (already required for
// the existing single-threaded path) is what makes moving _Func into the job always valid here
// too. The job is a plain heap allocation, deleted by whichever chunk finishes last -- no
// allocator-awareness, matching this file's own existing lack of it everywhere else; a
// deliberate Pass 2 simplification, not an oversight.
inline constexpr size_t __bulk_max_chunks = 32;

template <bool _Chunked, class _Shape, class _Func, class _Rcvr, class... _Args>
struct __bulk_parallel_job;

// A chunk's own receiver: runs the job's [__begin, __end) sub-range on whichever pool worker
// it lands on -- one call to f covering the whole sub-range for bulk_chunked_t, or one call to
// f per index within it for bulk_unchunked_t (see __bulk_parallel_job::__invoke_chunk) -- then
// reports back to the shared job. get_env() deliberately answers nothing (matching
// __inline_sender's own convention): this receiver never needs a stop token or allocator from
// its environment.
// set_stopped() is declared even though it's never actually reachable in practice (no stop
// token is ever propagated into this environment) -- confirmed via a real build that
// connect()'s own receiver_of check requires it regardless.
template <class _Job>
struct __bulk_chunk_rcvr {
  using receiver_concept = receiver_tag;

  _Job* __job;
  typename _Job::__shape_t __begin;
  typename _Job::__shape_t __end;

  _LIBCPP_HIDE_FROM_ABI void set_value() && noexcept { __job->__run_chunk(__begin, __end); }

  _LIBCPP_HIDE_FROM_ABI void set_stopped() && noexcept { __job->__chunk_stopped(); }

  _LIBCPP_HIDE_FROM_ABI auto get_env() const noexcept { return env<>{}; }
};

template <bool _Chunked, class _Shape, class _Func, class _Rcvr, class... _Args>
struct __bulk_parallel_job {
  using __shape_t    = _Shape;
  using __chunk_op_t = connect_result_t<__parallel_sender, __bulk_chunk_rcvr<__bulk_parallel_job>>;

  template <class... _UArgs>
  _LIBCPP_HIDE_FROM_ABI __bulk_parallel_job(
      _Func&& __f, _Shape __shape, _Rcvr&& __rcvr, parallel_scheduler __sch, _UArgs&&... __args)
      : __f_(std::move(__f)), __args_(std::forward<_UArgs>(__args)...), __rcvr_(std::move(__rcvr)) {
    size_t __hw       = std::thread::hardware_concurrency();
    size_t __workers  = __hw == 0 ? size_t(1) : static_cast<size_t>(__hw);
    size_t __n        = __shape == _Shape(0)
                          ? size_t(1)
                          : (std::min)((std::min)(static_cast<size_t>(__shape), __workers), __bulk_max_chunks);
    __remaining_.store(__n, std::memory_order_relaxed);
    __chunk_ops_.reserve(__n);
    _Shape __base   = static_cast<_Shape>(__shape / static_cast<_Shape>(__n));
    _Shape __rem    = static_cast<_Shape>(__shape % static_cast<_Shape>(__n));
    _Shape __cursor = _Shape(0);
    for (size_t __i = 0; __i < __n; ++__i) {
      _Shape __len   = static_cast<_Shape>(__base + (static_cast<_Shape>(__i) < __rem ? _Shape(1) : _Shape(0)));
      _Shape __begin = __cursor;
      _Shape __end   = static_cast<_Shape>(__cursor + __len);
      __cursor       = __end;
      __chunk_ops_.push_back(execution::connect(
          execution::schedule(__sch), __bulk_chunk_rcvr<__bulk_parallel_job>{this, __begin, __end}));
    }
  }

  _LIBCPP_HIDE_FROM_ABI void __start() noexcept {
    for (auto& __op : __chunk_ops_) {
      execution::start(__op);
    }
  }

  // Called from __bulk_chunk_rcvr::set_value(), i.e. on a pool worker thread. May delete
  // `this` (see the file-level comment above) -- nothing may touch any member after
  // __finish() returns.
  _LIBCPP_HIDE_FROM_ABI void __run_chunk(_Shape __begin, _Shape __end) noexcept {
    constexpr bool __nothrow = __bulk_nothrow_invocable<_Chunked, _Func, _Shape, _Args...>::value;
    if constexpr (__nothrow) {
      __invoke_chunk(__begin, __end);
    } else {
      try {
        __invoke_chunk(__begin, __end);
      } catch (...) {
        lock_guard<mutex> __lock(__err_mtx_);
        if (!__first_err_) {
          __first_err_ = std::current_exception();
        }
      }
    }
    if (__remaining_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
      __finish();
    }
  }

  // Never actually reached: no stop token is ever propagated into __bulk_chunk_rcvr's
  // environment (env<>{}), so parallel_scheduler's own schedule() sender never produces
  // set_stopped() for it -- but receiver_of's own check still requires this receiver declare
  // it, matching what a real build's diagnostics demanded. Counted the same as an ordinary
  // completion (no chunk of work was lost: the standard leaves what "stopped" even means for
  // an individual bulk chunk unspecified, and this path is unreachable in practice).
  _LIBCPP_HIDE_FROM_ABI void __chunk_stopped() noexcept {
    if (__remaining_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
      __finish();
    }
  }

private:
  _LIBCPP_HIDE_FROM_ABI void __invoke_chunk(_Shape __begin, _Shape __end) {
    if constexpr (_Chunked) {
      std::apply([&](_Args&... __a) { std::invoke(__f_, __begin, __end, __a...); }, __args_);
    } else {
      std::apply(
          [&](_Args&... __a) {
            for (_Shape __i = __begin; __i != __end; ++__i) {
              std::invoke(__f_, __i, __a...);
            }
          },
          __args_);
    }
  }

  _LIBCPP_HIDE_FROM_ABI void __finish() noexcept {
    if (__first_err_) {
      exception_ptr __err = std::move(__first_err_);
      execution::set_error(std::move(__rcvr_), std::move(__err));
    } else {
      std::apply([&](_Args&... __a) { execution::set_value(std::move(__rcvr_), std::move(__a)...); }, __args_);
    }
    delete this;
  }

  _Func __f_;
  tuple<_Args...> __args_;
  _Rcvr __rcvr_;
  atomic<size_t> __remaining_{0};
  mutex __err_mtx_;
  exception_ptr __first_err_;
  vector<__chunk_op_t> __chunk_ops_;
};

// Entry receiver directly connected to bulk_chunked's/bulk_unchunked's child sender: the one
// and only set_value() call it ever receives is what supplies the real _Args... (only known at
// that point, not at __bulk_sndr::connect() time), so this is where __bulk_parallel_job is
// actually created and dispatched.
template <bool _Chunked, class _Policy, class _Shape, class _Func, class _Rcvr>
class __bulk_parallel_rcvr {
public:
  using receiver_concept = receiver_tag;

  _LIBCPP_HIDE_FROM_ABI constexpr __bulk_parallel_rcvr(
      __bulk_data<_Policy, _Shape, _Func>&& __data, _Rcvr&& __rcvr, parallel_scheduler __sch)
      : __data_(std::move(__data)), __rcvr_(std::move(__rcvr)), __sch_(__sch) {}

  template <class... _Args>
  _LIBCPP_HIDE_FROM_ABI void set_value(_Args&&... __args) && noexcept {
    using __job_t = __bulk_parallel_job<_Chunked, _Shape, _Func, _Rcvr, decay_t<_Args>...>;
    auto* __job =
        new __job_t(std::move(__data_.f), __data_.shape, std::move(__rcvr_), __sch_, std::forward<_Args>(__args)...);
    __job->__start();
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
  __bulk_data<_Policy, _Shape, _Func> __data_;
  _Rcvr __rcvr_;
  parallel_scheduler __sch_;
};
#endif // _LIBCPP_HAS_THREADS

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
#if _LIBCPP_HAS_THREADS
    // Pass 2/3 (P2079R10): probe child's completion scheduler via the same __try_query
    // primitive <__execution/get_scheduler.h>'s own get_completion_scheduler_t uses
    // internally -- calling the full get_completion_scheduler CPO directly here would be
    // unsafe: with no fallback env supplied, its own body hard-fails (a static_assert, not a
    // SFINAE-friendly substitution failure) when the query isn't answered at all, exactly the
    // "immediate context" pitfall documented in docs/CXX26_GAPS.md's M1/M2 entries.
    // __try_query's two overloads are individually requires-constrained, so probing whether
    // this call is well-formed at all is genuinely SFINAE-safe. Both bulk_chunked_t and
    // bulk_unchunked_t take this branch identically -- __bulk_parallel_job's own _Chunked
    // dispatch (see __invoke_chunk above) is what makes each worker do the right thing per tag.
    if constexpr (requires {
                    execution::__try_query(execution::get_env(child), get_completion_scheduler<set_value_t>);
                  }) {
      using __child_sch_t =
          remove_cvref_t<decltype(execution::__try_query(execution::get_env(child), get_completion_scheduler<set_value_t>))>;
      if constexpr (same_as<__child_sch_t, parallel_scheduler>) {
        return execution::connect(
            std::move(child), __bulk_parallel_rcvr<_Chunked, _Policy, _Shape, _Func, remove_cvref_t<_Rcvr>>(
                                   std::move(data), std::forward<_Rcvr>(__rcvr),
                                   execution::__try_query(execution::get_env(child), get_completion_scheduler<set_value_t>)));
      } else {
        return execution::connect(std::move(child),
                                   __bulk_rcvr<_Chunked, _Policy, _Shape, _Func, remove_cvref_t<_Rcvr>>(
                                       std::move(data), std::forward<_Rcvr>(__rcvr)));
      }
    } else
#endif // _LIBCPP_HAS_THREADS
    {
      return execution::connect(std::move(child), __bulk_rcvr<_Chunked, _Policy, _Shape, _Func, remove_cvref_t<_Rcvr>>(
                                                        std::move(data), std::forward<_Rcvr>(__rcvr)));
    }
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

_LIBCPP_POP_MACROS

#endif // _LIBCPP___EXECUTION_BULK_H
