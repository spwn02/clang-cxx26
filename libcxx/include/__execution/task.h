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
#include <__concepts/convertible_to.h>
#include <__concepts/same_as.h>
#include <__config>
#include <__coroutine/coroutine_handle.h>
#include <__coroutine/noop_coroutine_handle.h>
#include <__coroutine/trivial_awaitables.h>
#include <__execution/affine.h>
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
#include <__type_traits/is_class.h>
#include <__type_traits/is_reference.h>
#include <__type_traits/is_object.h>
#include <__type_traits/is_array.h>
#include <__functional/reference_wrapper.h>
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

#if _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_THREADS

namespace execution {

// [exec.task.with.error]. A co_yield-able wrapper reporting an error completion without
// terminating the coroutine's own control flow the way an uncaught exception would.
template <class _Err>
struct with_error {
  using type = remove_cvref_t<_Err>;
  _LIBCPP_NO_UNIQUE_ADDRESS type error;
};

template <class _Err>
with_error(_Err) -> with_error<_Err>;

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
struct __task_scheduler_type_detect<_Environment, __task_void_t<typename _Environment::start_scheduler_type>> {
  using type = typename _Environment::start_scheduler_type;
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

template <class _Environment, class = void>
struct __task_error_types_detect { using type = completion_signatures<set_error_t(exception_ptr)>; };
template <class _Environment>
struct __task_error_types_detect<_Environment, __task_void_t<typename _Environment::error_types>> {
  using type = typename _Environment::error_types;
};
template <class _Sig>
struct __task_error_sig { static constexpr bool __valid = false; using type = monostate; };
template <class _Error>
struct __task_error_sig<set_error_t(_Error)> { static constexpr bool __valid = true; using type = _Error; };
template <class _Signatures>
struct __task_errors { static constexpr bool __valid = false; };
template <class... _Sigs>
struct __task_errors<completion_signatures<_Sigs...>> {
  static constexpr bool __valid = (__task_error_sig<_Sigs>::__valid && ...);
  using __types = tuple<typename __task_error_sig<_Sigs>::type...>;
  static constexpr bool __has_exception = (same_as<_Sigs, set_error_t(exception_ptr)> || ...);
  template <class _Value>
  using __completions = completion_signatures<_Value, _Sigs..., set_stopped_t()>;
  template <class _Error>
  static constexpr size_t __conversion_count = (size_t(0) + ... + size_t(is_convertible_v<_Error, typename __task_error_sig<_Sigs>::type>));
  template <class _Error>
  static consteval size_t __conversion_index() {
    constexpr bool __matches[] = {is_convertible_v<_Error, typename __task_error_sig<_Sigs>::type>..., false};
    for (size_t __i = 0; __i != sizeof...(_Sigs); ++__i)
      if (__matches[__i]) return __i;
    return sizeof...(_Sigs);
  }
};

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
template <class _Tp>
using __task_stored_t = conditional_t<is_void_v<_Tp>, __task_unit,
    conditional_t<is_reference_v<_Tp>, reference_wrapper<remove_reference_t<_Tp>>, _Tp>>;
template <class _Tp>
using __task_result_t = variant<monostate, __task_stored_t<_Tp>, exception_ptr>;


// The type-erased interface promise_type delivers its final result through: task::connect's
// opstate (below) implements this, forwarding to whatever concrete receiver it was connected
// with. A single uniform __complete_value signature (via __task_unit for T=void) avoids
// needing a separate specialization of this interface per T.
template <class _Tp, class _Environment>
struct __task_result_sink {
  using __arg_t = conditional_t<is_void_v<_Tp>, __task_unit, _Tp>;

  _LIBCPP_HIDE_FROM_ABI virtual _Environment& __environment() noexcept = 0;
  _LIBCPP_HIDE_FROM_ABI virtual __task_scheduler_type_t<_Environment> __start_scheduler() = 0;
  _LIBCPP_HIDE_FROM_ABI virtual __task_allocator_type_t<_Environment> __sink_allocator() = 0;
  using __token_t = decltype(std::declval<__task_stop_source_type_t<_Environment>&>().get_token());
  _LIBCPP_HIDE_FROM_ABI virtual __token_t __stop_token() = 0;
  _LIBCPP_HIDE_FROM_ABI virtual __task_result_t<_Tp>* __result_storage() noexcept = 0;
  _LIBCPP_HIDE_FROM_ABI virtual void __complete_value(__arg_t&&) noexcept    = 0;
  _LIBCPP_HIDE_FROM_ABI virtual void __complete_error(exception_ptr) noexcept = 0;
  _LIBCPP_HIDE_FROM_ABI virtual void __complete_custom(size_t, void*) noexcept = 0;
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
  using __result_variant_t = __task_result_t<_Tp>;
  __result_variant_t* __result_ = nullptr;

  template <class _Vp>
  _LIBCPP_HIDE_FROM_ABI void return_value(_Vp&& __v) {
    if constexpr (is_reference_v<_Tp>)
      __result_->template emplace<1>(__v);
    else
      __result_->template emplace<1>(std::forward<_Vp>(__v));
  }
};

template <>
struct __task_promise_result<void> {
  using __result_variant_t = __task_result_t<void>;
  __result_variant_t* __result_ = nullptr;

  _LIBCPP_HIDE_FROM_ABI void return_void() noexcept { __result_->template emplace<1>(__task_unit{}); }
};

template <class _Tp = void, class _Environment = env<>>
class task;

template <class _Environment, class _RcvrEnv, class = void>
struct __task_own_env { using type = env<>; };
template <class _Environment, class _RcvrEnv>
struct __task_own_env<_Environment, _RcvrEnv,
    __task_void_t<typename _Environment::template env_type<_RcvrEnv>>> {
  using type = typename _Environment::template env_type<_RcvrEnv>;
};

template <class _PromiseType, class _Tp, class _Environment, class _Rcvr>
class __task_opstate final : public __task_result_sink<_Tp, _Environment> {
  using __arg_t = typename __task_result_sink<_Tp, _Environment>::__arg_t;

public:
  using operation_state_concept = operation_state_tag;

  using __own_env_t = typename __task_own_env<_Environment, env_of_t<_Rcvr>>::type;
  using __source_t = __task_stop_source_type_t<_Environment>;
  using __token_t = typename __task_result_sink<_Tp, _Environment>::__token_t;
  template <class _Type, class _Arg>
  _LIBCPP_HIDE_FROM_ABI static _Type __make_env(_Arg&& __arg) {
    if constexpr (is_constructible_v<_Type, _Arg>)
      return _Type(std::forward<_Arg>(__arg));
    else
      return _Type();
  }
  _LIBCPP_HIDE_FROM_ABI _Environment __make_environment() {
    if constexpr (is_constructible_v<_Environment, __own_env_t&>)
      return _Environment(__own_env_);
    else
      return __make_env<_Environment>(execution::get_env(__rcvr_));
  }
  template <class _Receiver>
  _LIBCPP_HIDE_FROM_ABI __task_opstate(coroutine_handle<_PromiseType> __h, _Receiver&& __rcvr)
      : __coro_(__h), __rcvr_(std::forward<_Receiver>(__rcvr)),
        __own_env_(__make_env<__own_env_t>(execution::get_env(__rcvr_))),
        __environment_(__make_environment()) {
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
  _LIBCPP_HIDE_FROM_ABI _Environment& __environment() noexcept override { return __environment_; }
  _LIBCPP_HIDE_FROM_ABI __task_scheduler_type_t<_Environment> __start_scheduler() override {
    using _Scheduler = __task_scheduler_type_t<_Environment>;
    if constexpr (requires { _Scheduler(execution::get_start_scheduler(execution::get_env(__rcvr_))); })
      return _Scheduler(execution::get_start_scheduler(execution::get_env(__rcvr_)));
    else
      return __task_default_scheduler<_Environment>();
  }
  _LIBCPP_HIDE_FROM_ABI __task_allocator_type_t<_Environment> __sink_allocator() override {
    using _Allocator = __task_allocator_type_t<_Environment>;
    if constexpr (requires { _Allocator(std::get_allocator(execution::get_env(__rcvr_))); })
      return _Allocator(std::get_allocator(execution::get_env(__rcvr_)));
    else
      return _Allocator();
  }
  _LIBCPP_HIDE_FROM_ABI __token_t __stop_token() override {
    if constexpr (same_as<__token_t, decltype(std::get_stop_token(execution::get_env(__rcvr_)))>)
      return std::get_stop_token(execution::get_env(__rcvr_));
    else {
      if (std::get_stop_token(execution::get_env(__rcvr_)).stop_requested())
        __source_.request_stop();
      return __source_.get_token();
    }
  }
  _LIBCPP_HIDE_FROM_ABI __task_result_t<_Tp>* __result_storage() noexcept override { return &__result_; }
  _LIBCPP_HIDE_FROM_ABI void __destroy_frame() noexcept { std::exchange(__coro_, {}).destroy(); }
  _LIBCPP_HIDE_FROM_ABI void __complete_value(__arg_t&& __v) noexcept override {
    __destroy_frame();
    if constexpr (is_void_v<_Tp>) {
      (void)__v;
      execution::set_value(std::move(__rcvr_));
    } else {
      execution::set_value(std::move(__rcvr_), std::forward<__arg_t>(__v));
    }
  }
  _LIBCPP_HIDE_FROM_ABI void __complete_error(exception_ptr __ep) noexcept override {
    __destroy_frame();
    using _Errors = typename __task_error_types_detect<_Environment>::type;
    if constexpr (__task_errors<_Errors>::__has_exception)
      execution::set_error(std::move(__rcvr_), std::move(__ep));
    else
      std::terminate();
  }
  template <size_t _Idx = 0>
  _LIBCPP_HIDE_FROM_ABI void __dispatch_error(size_t __index, void* __error) noexcept {
    using _Errors = typename __task_errors<typename __task_error_types_detect<_Environment>::type>::__types;
    if constexpr (_Idx < tuple_size_v<_Errors>) {
      if (__index == _Idx) {
        using _Error = tuple_element_t<_Idx, _Errors>;
        _Error __value(std::move(*static_cast<_Error*>(__error)));
        __destroy_frame();
        execution::set_error(std::move(__rcvr_), std::move(__value));
      } else {
        __dispatch_error<_Idx + 1>(__index, __error);
      }
    } else {
      std::terminate();
    }
  }
  _LIBCPP_HIDE_FROM_ABI void __complete_custom(size_t __index, void* __error) noexcept override {
    __dispatch_error(__index, __error);
  }
  _LIBCPP_HIDE_FROM_ABI void __complete_stopped() noexcept override {
    __destroy_frame();
    execution::set_stopped(std::move(__rcvr_));
  }

  coroutine_handle<_PromiseType> __coro_;
  _Rcvr __rcvr_;
  __task_result_t<_Tp> __result_{};
  __own_env_t __own_env_;
  _Environment __environment_;
  __source_t __source_;
};

// [exec.task]
template <class _Tp, class _Environment>
class task {
  static_assert(is_void_v<_Tp> || is_reference_v<_Tp> ||
                (is_object_v<_Tp> && !is_array_v<_Tp> && same_as<_Tp, remove_cvref_t<_Tp>>),
                "task requires void, a reference, or a cv-unqualified non-array object type.");
  static_assert(is_class_v<_Environment>, "task requires a class Environment.");
public:
  using sender_concept    = sender_tag;
  using allocator_type    = __task_allocator_type_t<_Environment>;
  using start_scheduler_type = __task_scheduler_type_t<_Environment>;
  using scheduler_type = start_scheduler_type;
  using stop_source_type  = __task_stop_source_type_t<_Environment>;
  using stop_token_type   = decltype(std::declval<stop_source_type&>().get_token());
  using error_types = typename __task_error_types_detect<_Environment>::type;
  static_assert(__task_errors<error_types>::__valid, "task error_types must contain only set_error_t(E) signatures.");

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
  _LIBCPP_HIDE_FROM_ABI __task_opstate<promise_type, _Tp, _Environment, remove_cvref_t<_Rcvr>> connect(_Rcvr&& __rcvr) && {
    static_assert(requires { allocator_type(std::get_allocator(execution::get_env(__rcvr))); } ||
                      requires { allocator_type(); },
                  "Mandates: allocator_type(get_allocator(get_env(rcvr))) or allocator_type() is well-formed");
    return __task_opstate<promise_type, _Tp, _Environment, remove_cvref_t<_Rcvr>>(
        std::exchange(__coro_, {}), std::forward<_Rcvr>(__rcvr));
  }

  template <class _Self, class... _Env>
  _LIBCPP_HIDE_FROM_ABI static consteval auto get_completion_signatures() {
    return typename __task_errors<error_types>::template __completions<__set_value_sig_t<_Tp>>{};
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
      __p.__result_->template emplace<2>(std::move(__ep));
      __p.__deliver_result();
    }
    _LIBCPP_HIDE_FROM_ABI void set_stopped() && noexcept { __h_.promise().__sink_->__complete_stopped(); }
    _LIBCPP_HIDE_FROM_ABI auto get_env() const noexcept { return env<>{}; }
  };

  using __sched_op_t = connect_result_t<schedule_result_t<scheduler_type&>, __resume_rcvr>;

public:
  _LIBCPP_HIDE_FROM_ABI promise_type() = default;

  // P3980R1: the promise has no constructor taking the coroutine arguments; the allocator of the environment is the
  // allocator of the receiver (see __promise_env) and allocator_arg_t arguments only reach operator new.
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

  _LIBCPP_HIDE_FROM_ABI void unhandled_exception() noexcept {
    using _Errors = __task_errors<error_types>;
    if constexpr (_Errors::__has_exception)
      __result_->template emplace<2>(std::current_exception());
    else
      std::terminate();
  }

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
  _LIBCPP_HIDE_FROM_ABI auto yield_value(with_error<_Err> __e) {
    using _Errors = __task_errors<error_types>;
    using _Input = typename with_error<_Err>::type;
    static_assert(_Errors::template __conversion_count<_Input&&> == 1,
                  "with_error must be convertible to exactly one error_types argument type.");
    constexpr size_t __index = _Errors::template __conversion_index<_Input&&>();
    using _Error = tuple_element_t<__index, typename _Errors::__types>;
    struct __error_awaiter {
      _Error __error;
      _LIBCPP_HIDE_FROM_ABI bool await_ready() const noexcept { return false; }
      _LIBCPP_HIDE_FROM_ABI void await_suspend(coroutine_handle<promise_type> __h) noexcept {
        __h.promise().__sink_->__complete_custom(__index, std::addressof(__error));
      }
      _LIBCPP_HIDE_FROM_ABI void await_resume() noexcept {}
    };
    return __error_awaiter{_Error(std::move(__e.error))};
  }

  // Brings with_awaitable_senders's generic await_transform(Value&&) into this scope
  // alongside the overloads below -- without this, they would hide it entirely rather
  // than participate in the same overload set.
  using with_awaitable_senders<promise_type>::await_transform;

  // [exec.task.promise]: if the start scheduler is an inline_scheduler the sender is delivered the ordinary way
  // (as_awaitable), otherwise it is first adapted with affine so that it completes on the start scheduler of the
  // environment of this promise.
  template <sender _Sndr>
  _LIBCPP_HIDE_FROM_ABI decltype(auto) await_transform(_Sndr&& __sndr) {
    if constexpr (same_as<start_scheduler_type, inline_scheduler>)
      return execution::as_awaitable(std::forward<_Sndr>(__sndr), *this);
    else
      return execution::as_awaitable(execution::affine(std::forward<_Sndr>(__sndr)), *this);
  }

  struct __promise_env {
    const promise_type* __promise_;
    _LIBCPP_HIDE_FROM_ABI scheduler_type query(get_start_scheduler_t) const noexcept {
      return __promise_->__scheduler_;
    }
    _LIBCPP_HIDE_FROM_ABI allocator_type query(get_allocator_t) const noexcept {
      return __promise_->__sink_->__sink_allocator();
    }
    _LIBCPP_HIDE_FROM_ABI stop_token_type query(get_stop_token_t) const noexcept {
      return __promise_->__sink_->__stop_token();
    }
    template <class _Query, class... _Args>
      requires requires(_Environment& __e, _Query __q, _Args&&... __args) {
        __e.query(__q, std::forward<_Args>(__args)...);
      } && (forwarding_query(_Query{}))
    _LIBCPP_HIDE_FROM_ABI decltype(auto) query(_Query __q, _Args&&... __args) const
        noexcept(noexcept(__promise_->__sink_->__environment().query(__q, std::forward<_Args>(__args)...))) {
      return __promise_->__sink_->__environment().query(__q, std::forward<_Args>(__args)...);
    }
  };
  _LIBCPP_HIDE_FROM_ABI __promise_env get_env() const noexcept { return {this}; }

  // Called by task::connect's opstate (__task_opstate, above) once it knows where the final
  // result should go; not callable before that, matching a task's own connect-once contract.
  _LIBCPP_HIDE_FROM_ABI void __set_sink(__task_result_sink<_Tp, _Environment>* __sink) noexcept {
    __sink_ = __sink;
    __result_ = __sink->__result_storage();
  }

  // Called once by __task_opstate::start() (the real, receiver-triggered start of execution):
  // connects+starts schedule(__scheduler_), whose receiver resumes __h once that completes.
  // Not called from initial_suspend -- see that method's own comment for why.
  _LIBCPP_HIDE_FROM_ABI void __start(coroutine_handle<promise_type> __h) {
    __scheduler_ = __sink_->__start_scheduler();
    __h.resume();
  }
  struct __make_sched_op {
    promise_type* __p;
    coroutine_handle<promise_type> __h;
    _LIBCPP_HIDE_FROM_ABI operator __sched_op_t() const {
      return execution::connect(execution::schedule(__p->__scheduler_), __resume_rcvr{__h});
    }
  };
  _LIBCPP_HIDE_FROM_ABI void __schedule_resume(coroutine_handle<promise_type> __h) {
    __sched_op_.emplace(__make_sched_op{this, __h});
    execution::start(*__sched_op_);
  }

  // [exec.task.promise]: allocator-aware operator new/delete (P3980R1). The allocator is taken from an
  // allocator_arg_t-led argument list of the coroutine function (optionally preceded by the implicit object
  // parameter of a member-function coroutine). A single
  // non-template operator delete can't itself be templated on the (call-site-only-known)
  // allocator type, so the allocated block stores a small, fixed-layout header immediately
  // before the coroutine frame recording (a) a type-erased deallocation thunk and (b) how far
  // back the actual (variable-sized, rebound-to-byte) allocator object sits -- letting delete
  // locate and invoke both without ever needing to name the allocator type itself.
  _LIBCPP_HIDE_FROM_ABI static void* operator new(size_t __frame_sz) {
    return operator new(__frame_sz, allocator_arg, allocator_type());
  }
  template <class _Alloc, class... _Args>
  _LIBCPP_HIDE_FROM_ABI static void* operator new(size_t __frame_sz, allocator_arg_t, _Alloc __alloc, _Args&&...) {
    return __allocate_with(__frame_sz, __alloc);
  }
  template <class _This, class _Alloc, class... _Args>
  _LIBCPP_HIDE_FROM_ABI static void*
  operator new(size_t __frame_sz, const _This&, allocator_arg_t, _Alloc __alloc, _Args&&...) {
    return __allocate_with(__frame_sz, __alloc);
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

  struct alignas(__STDCPP_DEFAULT_NEW_ALIGNMENT__) __allocation_unit {
    byte __bytes_[__STDCPP_DEFAULT_NEW_ALIGNMENT__];
  };

  template <class _ByteAlloc>
  _LIBCPP_HIDE_FROM_ABI static void __dealloc_thunk(byte* __block, size_t __total) noexcept {
    _ByteAlloc __a(std::move(*reinterpret_cast<_ByteAlloc*>(__block)));
    reinterpret_cast<_ByteAlloc*>(__block)->~_ByteAlloc();
    allocator_traits<_ByteAlloc>::deallocate(__a, reinterpret_cast<__allocation_unit*>(__block), __total);
  }

  template <class _Alloc>
  _LIBCPP_HIDE_FROM_ABI static void* __allocate_with(size_t __frame_sz, const _Alloc& __alloc) {
    using _ByteAlloc      = typename allocator_traits<_Alloc>::template rebind_alloc<__allocation_unit>;
    constexpr size_t __al = alignof(__frame_header);
    size_t __block_offset = ((sizeof(_ByteAlloc) + __al - 1) / __al) * __al;
    size_t __total        = (__block_offset + sizeof(__frame_header) + __frame_sz + sizeof(__allocation_unit) - 1) / sizeof(__allocation_unit);

    _ByteAlloc __a(__alloc);
    static_assert(is_pointer_v<typename allocator_traits<_ByteAlloc>::pointer>);
    byte* __block = reinterpret_cast<byte*>(allocator_traits<_ByteAlloc>::allocate(__a, __total));
    ::new (static_cast<void*>(__block)) _ByteAlloc(__a);
    auto* __hdr = ::new (static_cast<void*>(__block + __block_offset))
        __frame_header{&__dealloc_thunk<_ByteAlloc>, __total, __block_offset};
    return reinterpret_cast<byte*>(__hdr) + sizeof(__frame_header);
  }

  _LIBCPP_HIDE_FROM_ABI void __deliver_result() noexcept {
    if (__result_->index() == 2) {
      __sink_->__complete_error(std::get<2>(std::move(*__result_)));
    } else {
      if constexpr (is_reference_v<_Tp>)
        __sink_->__complete_value(static_cast<_Tp&&>(std::get<1>(*__result_).get()));
      else
        __sink_->__complete_value(std::get<1>(std::move(*__result_)));
    }
  }

  scheduler_type __scheduler_ = __task_default_scheduler<_Environment>();
  stop_source_type __stop_source_{};
  __task_result_sink<_Tp, _Environment>* __sink_ = nullptr;
  optional<__sched_op_t> __sched_op_{};
};

} // namespace execution

#endif // _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_THREADS

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP___EXECUTION_TASK_H
