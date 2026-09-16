//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___EXECUTION_TASK_H
#define _LIBCPP___EXECUTION_TASK_H

#include <__concepts/constructible.h>
#include <__concepts/same_as.h>
#include <__config>
#include <__coroutine/coroutine_handle.h>
#include <__coroutine/noop_coroutine_handle.h>
#include <__coroutine/trivial_awaitables.h>
#include <__execution/affine_on.h>
#include <__execution/completion_functions.h>
#include <__execution/completion_signatures.h>
#include <__execution/connect.h>
#include <__execution/env.h>
#include <__execution/get_allocator.h>
#include <__execution/get_completion_signatures.h>
#include <__execution/get_env.h>
#include <__execution/get_scheduler.h>
#include <__execution/get_stop_token.h>
#include <__execution/operation_state.h>
#include <__execution/receiver.h>
#include <__execution/schedule.h>
#include <__execution/scheduler.h>
#include <__execution/sender.h>
#include <__execution/task_scheduler.h>
#include <__execution/with_awaitable_senders.h>
#include <__memory/allocator.h>
#include <__memory/allocator_arg_t.h>
#include <__memory/allocator_traits.h>
#include <__stop_token/inplace_stop_source.h>
#include <__stop_token/inplace_stop_token.h>
#include <__type_traits/conditional.h>
#include <__type_traits/is_void.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/exchange.h>
#include <__utility/forward.h>
#include <__utility/move.h>
#include <cstddef>
#include <exception>
#include <new>
#include <optional>
#include <variant>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_PUSH_MACROS
#include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

// execution::task (P3552R3, GitHub issue #12). See docs/design/execution_task_p3552.md for the
// design note this implementation follows, including two deliberate scope cuts made explicit
// there: (1) a custom Environment::error_types is not supported -- every task<T, Environment>
// completes with exactly set_error_t(exception_ptr) regardless of Environment; (2)
// affine_on's same-resource fast path (an optimization, confirmed via the adopted wording:
// "the completion operation ... *may* be called before start(op) completes") is not
// implemented -- every scheduling hop is taken unconditionally via continues_on. Both are
// documented, bounded simplifications, not silent gaps.
#if _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_THREADS

namespace execution {

// [exec.task.with.error]. A co_yield-able wrapper reporting an error completion without
// terminating the coroutine's own control flow the way an uncaught exception would.
template <class _Err>
struct with_error {
  _LIBCPP_NO_UNIQUE_ADDRESS _Err error;
};

// [exec.task.change.sched]. An explicit await_transform target letting a task's body switch
// which scheduler it resumes on for the remainder of its execution.
template <class _Sch>
struct change_coroutine_scheduler {
  _LIBCPP_NO_UNIQUE_ADDRESS _Sch scheduler;
};

// Detection idiom for Environment's optional nested types. allocator_type/scheduler_type/
// stop_source_type are genuinely customizable, matching [exec.task.type]; error_types is
// deliberately NOT detected here -- see this file's top comment.
template <class...>
using __task_void_t = void;

template <class _Environment, class = void>
struct __task_allocator_type {
  using type = allocator<byte>;
};
template <class _Environment>
struct __task_allocator_type<_Environment, __task_void_t<typename _Environment::allocator_type>> {
  using type = typename _Environment::allocator_type;
};
template <class _Environment>
using __task_allocator_type_t = typename __task_allocator_type<_Environment>::type;

template <class _Environment, class = void>
struct __task_scheduler_type_detect {
  using type = task_scheduler;
};
template <class _Environment>
struct __task_scheduler_type_detect<_Environment, __task_void_t<typename _Environment::scheduler_type>> {
  using type = typename _Environment::scheduler_type;
};
template <class _Environment>
using __task_scheduler_type_t = typename __task_scheduler_type_detect<_Environment>::type;

template <class _Environment, class = void>
struct __task_stop_source_type_detect {
  using type = inplace_stop_source;
};
template <class _Environment>
struct __task_stop_source_type_detect<_Environment, __task_void_t<typename _Environment::stop_source_type>> {
  using type = typename _Environment::stop_source_type;
};
template <class _Environment>
using __task_stop_source_type_t = typename __task_stop_source_type_detect<_Environment>::type;

// This pass's one fixed point (see this file's top comment): every task<T, Environment>
// completes with exactly this error shape, regardless of Environment.
using __task_error_types = completion_signatures<set_error_t(exception_ptr)>;

// task_scheduler is not default-constructible (see task_scheduler.h), so a default
// scheduler_type value needs an explicit construction site; a custom Environment::scheduler_type
// is assumed default-constructible (a reasonable requirement on the customization point itself).
template <class _Environment>
_LIBCPP_HIDE_FROM_ABI __task_scheduler_type_t<_Environment> __task_default_scheduler() {
  if constexpr (same_as<__task_scheduler_type_t<_Environment>, task_scheduler>) {
    return task_scheduler(inline_scheduler{});
  } else {
    return __task_scheduler_type_t<_Environment>();
  }
}

// A stand-in for T when T is void, so the result-storage variant and the sink interface below
// always have a real "value" alternative/argument to name -- mirrors <__execution/as_awaitable.h>'s
// own __unit for exactly the same reason.
struct __task_unit {};

// The type-erased interface promise_type delivers its final result through: task::connect's
// opstate (below) implements this, forwarding to whatever concrete receiver it was connected
// with. A single uniform __complete_value signature (via __task_unit for T=void) avoids
// needing a separate specialization of this interface per T.
template <class _Tp>
struct __task_result_sink {
  using __arg_t = conditional_t<is_void_v<_Tp>, __task_unit, _Tp>;

  _LIBCPP_HIDE_FROM_ABI virtual void __complete_value(__arg_t&&) noexcept    = 0;
  _LIBCPP_HIDE_FROM_ABI virtual void __complete_error(exception_ptr) noexcept = 0;
  _LIBCPP_HIDE_FROM_ABI virtual void __complete_stopped() noexcept          = 0;

protected:
  _LIBCPP_HIDE_FROM_ABI ~__task_result_sink() = default;
};

// A promise_type may declare return_void() XOR return_value(V&&), never both -- and, unlike an
// ordinary member function, this rule is enforced by whether the *name* return_void/
// return_value is found by ordinary lookup within the class, not by whether some overload of
// it would actually be viable for a given instantiation. A single promise_type with both
// declared side by side, even if mutually exclusived via `requires(is_void_v<T>)`/
// `requires(!is_void_v<T>)`, still declares both names and is rejected. Splitting the
// result-storage member *and* whichever one of the two methods applies into a template
// specialized on T (inherited by promise_type below) means the non-applicable name genuinely
// does not exist for a given T, not merely a constrained-away overload of it.
template <class _Tp>
struct __task_promise_result {
  using __result_variant_t = variant<monostate, _Tp, exception_ptr>;
  __result_variant_t __result_{};

  template <class _Vp>
  _LIBCPP_HIDE_FROM_ABI void return_value(_Vp&& __v) {
    try {
      __result_.template emplace<1>(std::forward<_Vp>(__v));
    } catch (...) {
      __result_.template emplace<2>(std::current_exception());
    }
  }
};

template <>
struct __task_promise_result<void> {
  using __result_variant_t = variant<monostate, __task_unit, exception_ptr>;
  __result_variant_t __result_{};

  _LIBCPP_HIDE_FROM_ABI void return_void() noexcept { __result_.template emplace<1>(__task_unit{}); }
};

template <class _Tp = void, class _Environment = void>
class task;

template <class _PromiseType, class _Tp, class _Rcvr>
class __task_opstate final : public __task_result_sink<_Tp> {
  using __arg_t = typename __task_result_sink<_Tp>::__arg_t;

public:
  using operation_state_concept = operation_state_tag;

  _LIBCPP_HIDE_FROM_ABI __task_opstate(coroutine_handle<_PromiseType> __h, _Rcvr&& __rcvr)
      : __coro_(__h), __rcvr_(std::move(__rcvr)) {
    __coro_.promise().__set_sink(this);
  }

  __task_opstate(const __task_opstate&)            = delete;
  __task_opstate& operator=(const __task_opstate&) = delete;

  _LIBCPP_HIDE_FROM_ABI ~__task_opstate() {
    if (__coro_)
      __coro_.destroy();
  }

  // [exec.task.state]: start(op) is what actually begins execution -- calling the coroutine
  // function only created a suspended frame (initial_suspend always suspends; see
  // promise_type::initial_suspend's own comment). This is the one and only call to
  // promise_type::__start, which arranges resumption through the task's scheduler and, once
  // that completes, resumes the coroutine body for the first time.
  _LIBCPP_HIDE_FROM_ABI void start() & noexcept { __coro_.promise().__start(__coro_); }

private:
  _LIBCPP_HIDE_FROM_ABI void __complete_value(__arg_t&& __v) noexcept override {
    if constexpr (is_void_v<_Tp>) {
      (void)__v;
      execution::set_value(std::move(__rcvr_));
    } else {
      execution::set_value(std::move(__rcvr_), std::move(__v));
    }
  }
  _LIBCPP_HIDE_FROM_ABI void __complete_error(exception_ptr __ep) noexcept override {
    execution::set_error(std::move(__rcvr_), std::move(__ep));
  }
  _LIBCPP_HIDE_FROM_ABI void __complete_stopped() noexcept override { execution::set_stopped(std::move(__rcvr_)); }

  coroutine_handle<_PromiseType> __coro_;
  _Rcvr __rcvr_;
};

// [exec.task]
template <class _Tp, class _Environment>
class task {
public:
  using sender_concept    = sender_tag;
  using allocator_type    = __task_allocator_type_t<_Environment>;
  using scheduler_type    = __task_scheduler_type_t<_Environment>;
  using stop_source_type  = __task_stop_source_type_t<_Environment>;
  using stop_token_type   = decltype(std::declval<stop_source_type&>().get_token());
  using error_types       = __task_error_types;

  class promise_type;

  _LIBCPP_HIDE_FROM_ABI task(task&& __other) noexcept : __coro_(std::exchange(__other.__coro_, {})) {}
  task(const task&)            = delete;
  task& operator=(const task&) = delete;
  task& operator=(task&&)      = delete;

  _LIBCPP_HIDE_FROM_ABI ~task() {
    if (__coro_)
      __coro_.destroy();
  }

  template <receiver _Rcvr>
  _LIBCPP_HIDE_FROM_ABI __task_opstate<promise_type, _Tp, remove_cvref_t<_Rcvr>> connect(_Rcvr&& __rcvr) && {
    return __task_opstate<promise_type, _Tp, remove_cvref_t<_Rcvr>>(
        std::exchange(__coro_, {}), std::forward<_Rcvr>(__rcvr));
  }

  template <class _Self, class _Env>
  _LIBCPP_HIDE_FROM_ABI static consteval auto get_completion_signatures() {
    return completion_signatures<__set_value_sig_t<_Tp>, set_error_t(exception_ptr), set_stopped_t()>{};
  }

private:
  friend class promise_type;

  _LIBCPP_HIDE_FROM_ABI explicit task(coroutine_handle<promise_type> __h) noexcept : __coro_(__h) {}

  coroutine_handle<promise_type> __coro_;
};

// [exec.task.promise]
template <class _Tp, class _Environment>
class task<_Tp, _Environment>::promise_type : public with_awaitable_senders<promise_type>,
                                               public __task_promise_result<_Tp> {
  // __result_ is declared on the dependent base __task_promise_result<_Tp> (return_void/
  // return_value live there too, split by specialization -- see that template's own comment);
  // this brings the name into non-dependent scope so the rest of this class's member function
  // bodies can keep referring to it unqualified.
  using __task_promise_result<_Tp>::__result_;

  // The receiver used to connect schedule(__scheduler_): resumes the coroutine on set_value
  // (the ordinary, expected path for every scheduler in this fork today); a scheduling
  // failure/cancellation (only possible for a future fallible scheduler, P2079R10) delivers
  // straight to the sink without ever resuming the coroutine body.
  struct __resume_rcvr {
    using receiver_concept = receiver_tag;

    coroutine_handle<promise_type> __h_;

    _LIBCPP_HIDE_FROM_ABI void set_value() && noexcept { __h_.resume(); }
    _LIBCPP_HIDE_FROM_ABI void set_error(exception_ptr __ep) && noexcept {
      promise_type& __p = __h_.promise();
      __p.__result_.template emplace<2>(std::move(__ep));
      __p.__deliver_result();
    }
    _LIBCPP_HIDE_FROM_ABI void set_stopped() && noexcept { __h_.promise().__sink_->__complete_stopped(); }
    _LIBCPP_HIDE_FROM_ABI auto get_env() const noexcept { return env<>{}; }
  };

  using __sched_op_t = connect_result_t<schedule_result_t<scheduler_type&>, __resume_rcvr>;

public:
  _LIBCPP_HIDE_FROM_ABI promise_type() = default;

  // [exec.task.promise]: allocator-extracting constructors, matching the argument-matching
  // convention operator new (below) uses -- if the coroutine function's own argument list
  // begins with allocator_arg_t + an allocator (optionally preceded by an implicit object
  // parameter for a member-function coroutine), this constructor initializes __alloc_ from it.
  template <class _Alloc, class... _Args>
    requires constructible_from<allocator_type, const _Alloc&>
  _LIBCPP_HIDE_FROM_ABI promise_type(allocator_arg_t, const _Alloc& __alloc, const _Args&...) noexcept(
      is_nothrow_constructible_v<allocator_type, const _Alloc&>)
      : __alloc_(__alloc) {}

  template <class _This, class _Alloc, class... _Args>
    requires constructible_from<allocator_type, const _Alloc&>
  _LIBCPP_HIDE_FROM_ABI promise_type(_This&, allocator_arg_t, const _Alloc& __alloc, const _Args&...) noexcept(
      is_nothrow_constructible_v<allocator_type, const _Alloc&>)
      : __alloc_(__alloc) {}

  _LIBCPP_HIDE_FROM_ABI task get_return_object() noexcept {
    return task(coroutine_handle<promise_type>::from_promise(*this));
  }

  // [exec.task.promise]: initial_suspend always suspends -- genuinely: creating/calling the
  // coroutine function only builds a suspended frame and returns a task object, it must not
  // start running the body. The scheduling hop ("arranges resumption on the scheduler") is
  // instead triggered from __start (below), which task::connect's opstate calls once (and
  // only once) an actual receiver's start() has been invoked; putting the scheduling logic
  // directly in this awaiter's await_suspend would instead run it immediately, during the
  // coroutine's very first invocation, before connect() ever happens.
  _LIBCPP_HIDE_FROM_ABI suspend_always initial_suspend() noexcept { return {}; }

  struct __final_awaiter {
    _LIBCPP_HIDE_FROM_ABI static constexpr bool await_ready() noexcept { return false; }
    _LIBCPP_HIDE_FROM_ABI coroutine_handle<> await_suspend(coroutine_handle<promise_type> __h) noexcept {
      __h.promise().__deliver_result();
      return noop_coroutine();
    }
    _LIBCPP_HIDE_FROM_ABI void await_resume() noexcept {}
  };
  _LIBCPP_HIDE_FROM_ABI __final_awaiter final_suspend() noexcept { return {}; }

  _LIBCPP_HIDE_FROM_ABI void unhandled_exception() noexcept { __result_.template emplace<2>(std::current_exception()); }

  // Overrides with_awaitable_senders's terminate-by-default hook: a sender co_await-ed from
  // within the coroutine body that completes with set_stopped() delivers set_stopped straight
  // to whatever this task is connected to, without running any more of the coroutine body.
  _LIBCPP_HIDE_FROM_ABI coroutine_handle<> unhandled_stopped() noexcept {
    __sink_->__complete_stopped();
    return noop_coroutine();
  }

  // [exec.task.with.error]: co_yield with_error<Err>{e} reports an error completion (as if the
  // coroutine had returned by throwing e, but without unwinding the coroutine's own stack) and
  // then immediately proceeds to final_suspend.
  template <class _Err>
  _LIBCPP_HIDE_FROM_ABI __final_awaiter yield_value(with_error<_Err> __e) noexcept {
    __result_.template emplace<2>(execution::__as_except_ptr(std::move(__e.error)));
    return {};
  }

  // [exec.task.change.sched]: co_await change_coroutine_scheduler{sch} switches which
  // scheduler the remainder of the coroutine body resumes on.
  _LIBCPP_HIDE_FROM_ABI auto await_transform(change_coroutine_scheduler<scheduler_type> __c) noexcept {
    struct __awaiter {
      promise_type* __p_;
      scheduler_type __new_sch_;
      _LIBCPP_HIDE_FROM_ABI static constexpr bool await_ready() noexcept { return false; }
      _LIBCPP_HIDE_FROM_ABI void await_suspend(coroutine_handle<promise_type> __h) noexcept {
        __p_->__scheduler_ = std::move(__new_sch_);
        __p_->__start(__h);
      }
      _LIBCPP_HIDE_FROM_ABI void await_resume() noexcept {}
    };
    return __awaiter{this, std::move(__c.scheduler)};
  }

  // Brings with_awaitable_senders's generic await_transform(Value&&) into this scope
  // alongside the two overloads below -- without this, they would hide it entirely rather
  // than participate in the same overload set.
  using with_awaitable_senders<promise_type>::await_transform;

  // [exec.task.promise]: a sender co_await-ed from within the coroutine body completes on
  // __scheduler_ (via affine_on), then is delivered the ordinary way (as_awaitable, via the
  // base class's await_transform). affine_on(sndr, __scheduler_) is unconditional here even
  // when __scheduler_ is already an inline one -- the paper's own same-resource fast path is a
  // documented, deferred optimization (see this file's top comment), not a correctness
  // requirement.
  template <sender _Sndr>
  _LIBCPP_HIDE_FROM_ABI decltype(auto) await_transform(_Sndr&& __sndr) {
    return with_awaitable_senders<promise_type>::await_transform(
        execution::affine_on(std::forward<_Sndr>(__sndr), __scheduler_));
  }

  _LIBCPP_HIDE_FROM_ABI auto get_env() const noexcept {
    return execution::env(
        execution::prop(execution::get_scheduler, __scheduler_),
        execution::prop(std::get_allocator, __alloc_),
        execution::prop(std::get_stop_token, __stop_source_.get_token()));
  }

  // Called by task::connect's opstate (__task_opstate, above) once it knows where the final
  // result should go; not callable before that, matching a task's own connect-once contract.
  _LIBCPP_HIDE_FROM_ABI void __set_sink(__task_result_sink<_Tp>* __sink) noexcept { __sink_ = __sink; }

  // Called once by __task_opstate::start() (the real, receiver-triggered start of execution)
  // and again by change_coroutine_scheduler's awaiter (to switch schedulers mid-body):
  // connects+starts schedule(__scheduler_), whose receiver resumes __h once that completes.
  // Not called from initial_suspend -- see that method's own comment for why.
  _LIBCPP_HIDE_FROM_ABI void __start(coroutine_handle<promise_type> __h) {
    __sched_op_.emplace(execution::connect(execution::schedule(__scheduler_), __resume_rcvr{__h}));
    execution::start(*__sched_op_);
  }

  // [exec.task.promise]: allocator-aware operator new/delete, extracting the allocator from an
  // allocator_arg_t-led argument list the same way the constructors above do. A single
  // non-template operator delete can't itself be templated on the (call-site-only-known)
  // allocator type, so the allocated block stores a small, fixed-layout header immediately
  // before the coroutine frame recording (a) a type-erased deallocation thunk and (b) how far
  // back the actual (variable-sized, rebound-to-byte) allocator object sits -- letting delete
  // locate and invoke both without ever needing to name the allocator type itself.
  template <class _Alloc, class... _Args>
  _LIBCPP_HIDE_FROM_ABI static void* operator new(size_t __frame_sz, allocator_arg_t, const _Alloc& __alloc, _Args&...) {
    return __allocate_with(__frame_sz, __alloc);
  }
  template <class _This, class _Alloc, class... _Args>
  _LIBCPP_HIDE_FROM_ABI static void*
  operator new(size_t __frame_sz, _This&, allocator_arg_t, const _Alloc& __alloc, _Args&...) {
    return __allocate_with(__frame_sz, __alloc);
  }
  template <class... _Args>
    requires default_initializable<allocator_type>
  _LIBCPP_HIDE_FROM_ABI static void* operator new(size_t __frame_sz, _Args&...) {
    return __allocate_with(__frame_sz, allocator_type());
  }

  _LIBCPP_HIDE_FROM_ABI static void operator delete(void* __p, size_t) noexcept {
    auto* __hdr    = reinterpret_cast<__frame_header*>(static_cast<byte*>(__p) - sizeof(__frame_header));
    byte* __block  = reinterpret_cast<byte*>(__hdr) - __hdr->__block_offset;
    size_t __total = __hdr->__block_size;
    auto __dealloc = __hdr->__dealloc;
    __hdr->~__frame_header();
    __dealloc(__block, __total);
  }

private:
  friend struct __resume_rcvr;

  struct alignas(alignof(::max_align_t)) __frame_header {
    void (*__dealloc)(byte*, size_t) noexcept;
    size_t __block_size;
    size_t __block_offset;
  };

  template <class _ByteAlloc>
  _LIBCPP_HIDE_FROM_ABI static void __dealloc_thunk(byte* __block, size_t __total) noexcept {
    _ByteAlloc __a(std::move(*reinterpret_cast<_ByteAlloc*>(__block)));
    reinterpret_cast<_ByteAlloc*>(__block)->~_ByteAlloc();
    allocator_traits<_ByteAlloc>::deallocate(__a, __block, __total);
  }

  template <class _Alloc>
  _LIBCPP_HIDE_FROM_ABI static void* __allocate_with(size_t __frame_sz, const _Alloc& __alloc) {
    using _ByteAlloc      = typename allocator_traits<_Alloc>::template rebind_alloc<byte>;
    constexpr size_t __al = alignof(__frame_header);
    size_t __block_offset = ((sizeof(_ByteAlloc) + __al - 1) / __al) * __al;
    size_t __total        = __block_offset + sizeof(__frame_header) + __frame_sz;

    _ByteAlloc __a(__alloc);
    byte* __block = allocator_traits<_ByteAlloc>::allocate(__a, __total);
    ::new (static_cast<void*>(__block)) _ByteAlloc(__a);
    auto* __hdr = ::new (static_cast<void*>(__block + __block_offset))
        __frame_header{&__dealloc_thunk<_ByteAlloc>, __total, __block_offset};
    return reinterpret_cast<byte*>(__hdr) + sizeof(__frame_header);
  }

  _LIBCPP_HIDE_FROM_ABI void __deliver_result() noexcept {
    if (__result_.index() == 2) {
      __sink_->__complete_error(std::get<2>(std::move(__result_)));
    } else {
      __sink_->__complete_value(std::get<1>(std::move(__result_)));
    }
  }

  scheduler_type __scheduler_ = __task_default_scheduler<_Environment>();
  allocator_type __alloc_{};
  stop_source_type __stop_source_{};
  __task_result_sink<_Tp>* __sink_ = nullptr;
  optional<__sched_op_t> __sched_op_{};
};

} // namespace execution

#endif // _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_THREADS

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP___EXECUTION_TASK_H
