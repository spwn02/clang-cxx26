// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___IO_BRIDGE_AS_SENDER_H
#define _LIBCPP___IO_BRIDGE_AS_SENDER_H

#include <__concepts/convertible_to.h>
#include <__config>
#include <__coroutine/coroutine_handle.h>
#include <__coroutine/trivial_awaitables.h>
#include <__exception/exception_ptr.h>
#include <__execution/completion_signatures.h>
#include <__execution/forwarding_query.h>
#include <__execution/get_env.h>
#include <__execution/get_stop_token.h>
#include <__execution/operation_state.h>
#include <__execution/receiver.h>
#include <__execution/sender.h>
#include <__io/continuation.h>
#include <__io/executor.h>
#include <__io/executor_ref.h>
#include <__io/frame_allocator.h>
#include <__io/io_awaitable.h>
#include <__io/io_env.h>
#include <__memory/addressof.h>
#include <__memory_resource/memory_resource.h>
#include <__stop_token/stop_callback.h>
#include <__stop_token/stop_source.h>
#include <__stop_token/stop_token.h>
#include <__stop_token/stoppable_token.h>
#include <__system_error/error_code.h>
#include <__tuple/tuple_size.h>
#include <__type_traits/conditional.h>
#include <__type_traits/decay.h>
#include <__type_traits/integral_constant.h>
#include <__type_traits/is_constructible.h>
#include <__type_traits/is_same.h>
#include <__type_traits/is_void.h>
#include <__type_traits/remove_cvref.h>
#include <__type_traits/remove_reference.h>
#include <__utility/declval.h>
#include <__utility/exchange.h>
#include <__utility/forward.h>
#include <__utility/move.h>
#include <optional>
#include <tuple>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_EXPERIMENTAL_NETWORKING && _LIBCPP_HAS_THREADS

_LIBCPP_PUSH_MACROS
#  include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

// P4093R1 appendix A: the query by which the receiver's environment names the executor the wrapped awaitable runs on.
// It is a forwarding query, so that it reaches the operation through adaptors such as then and let_value.
struct get_io_executor_t : forwarding_query_t {
  template <class _Env>
  _LIBCPP_HIDE_FROM_ABI auto operator()(const _Env& __env) const noexcept -> decltype(__env.query(*this)) {
    return __env.query(*this);
  }
};
inline constexpr get_io_executor_t get_io_executor{};

// P4093R1 appendix A: a ready-made environment for connecting an as_sender sender: the executor to run on and a stop
// token. The executor must outlive the operation.
struct io_sender_env {
  executor_ref io_executor;
  std::stop_token stop_token;

  _LIBCPP_HIDE_FROM_ABI executor_ref query(get_io_executor_t) const noexcept { return io_executor; }
  _LIBCPP_HIDE_FROM_ABI std::stop_token query(std::get_stop_token_t) const noexcept { return stop_token; }
};

// An awaitable result that is a binary outcome: a bare error_code or a single-element tuple-like of error_code.
template <class _Tp, class = void>
inline constexpr bool __has_tuple_protocol_v = false;
template <class _Tp>
inline constexpr bool __has_tuple_protocol_v<_Tp, void_t<typename tuple_size<_Tp>::type>> =
    (tuple_size<_Tp>::value > 0);

template <class _Tp, bool = __has_tuple_protocol_v<_Tp>>
inline constexpr bool __is_ec_outcome_v = is_same_v<_Tp, error_code>;
template <class _Tp>
inline constexpr bool __is_ec_outcome_v<_Tp, true> =
    tuple_size<_Tp>::value == 1 && is_same_v<remove_cvref_t<tuple_element_t<0, _Tp>>, error_code>;

// A compound result (error_code, more...): below the abstraction floor (P4093R1 section 4).
template <class _Tp, bool = __has_tuple_protocol_v<_Tp>>
inline constexpr bool __is_compound_ec_result_v = false;
template <class _Tp>
inline constexpr bool __is_compound_ec_result_v<_Tp, true> =
    tuple_size<_Tp>::value >= 2 && is_same_v<remove_cvref_t<tuple_element_t<0, _Tp>>, error_code>;

template <class _Aw>
using __bridge_await_result_t _LIBCPP_NODEBUG = decltype(std::declval<remove_cvref_t<_Aw>&>().await_resume());

enum class __as_sender_kind : unsigned char { __none, __value, __error_code, __exception, __stopped };

struct __as_sender_void {};

// The bridge between a stoppable_token of the receiver's environment and the std::stop_token of the io_env. A
// std::stop_token is used as it is and an unstoppable token gives an empty stop_token; any other token (the
// inplace_stop_token of when_all, for example) is forwarded through a stop_source owned by the operation state.
template <class _Tok, bool = is_same_v<_Tok, std::stop_token> || unstoppable_token<_Tok>>
struct __stop_bridge {
  struct __request {
    stop_source* __src_;
    _LIBCPP_HIDE_FROM_ABI void operator()() const noexcept { __src_->request_stop(); }
  };
  stop_source __source_;
  optional<typename _Tok::template callback_type<__request>> __cb_;

  _LIBCPP_HIDE_FROM_ABI std::stop_token __attach(const _Tok& __tok) {
    if (!__tok.stop_possible())
      return {};
    __cb_.emplace(__tok, __request{&__source_});
    return __source_.get_token();
  }
};
template <class _Tok>
struct __stop_bridge<_Tok, true> {
  _LIBCPP_HIDE_FROM_ABI std::stop_token __attach(const _Tok& __tok) {
    if constexpr (is_same_v<_Tok, std::stop_token>)
      return __tok;
    else
      return {};
  }
};

// The coroutine that awaits the IoAwaitable on behalf of the sender's operation state. It stores the outcome in the
// operation state and completes the receiver from final_suspend, where the frame is suspended: a receiver may destroy
// the operation state (and with it this frame) from inside the completion.
template <class _Op>
struct __bridge_task {
  struct promise_type {
    _Op* __op_           = nullptr;
    const io_env* __env_ = nullptr;

    struct __final_awaiter {
      _LIBCPP_HIDE_FROM_ABI bool await_ready() const noexcept { return false; }
      _LIBCPP_HIDE_FROM_ABI void await_suspend(coroutine_handle<promise_type> __h) const noexcept {
        __h.promise().__op_->__complete();
      }
      _LIBCPP_HIDE_FROM_ABI void await_resume() const noexcept {}
    };

    template <class _Aw>
    struct __env_awaiter {
      _Aw& __a_;
      const io_env* __env_;
      _LIBCPP_HIDE_FROM_ABI bool await_ready() { return __a_.await_ready(); }
      _LIBCPP_HIDE_FROM_ABI decltype(auto) await_suspend(coroutine_handle<> __h) {
        set_cached_frame_allocator(__env_->frame_allocator);
        return __a_.await_suspend(__h, __env_);
      }
      _LIBCPP_HIDE_FROM_ABI decltype(auto) await_resume() {
        set_cached_frame_allocator(__env_->frame_allocator);
        return __a_.await_resume();
      }
    };

    _LIBCPP_HIDE_FROM_ABI __bridge_task get_return_object() noexcept {
      return __bridge_task{coroutine_handle<promise_type>::from_promise(*this)};
    }
    _LIBCPP_HIDE_FROM_ABI suspend_always initial_suspend() noexcept { return {}; }
    _LIBCPP_HIDE_FROM_ABI __final_awaiter final_suspend() noexcept { return {}; }
    _LIBCPP_HIDE_FROM_ABI void return_void() noexcept {}
    _LIBCPP_HIDE_FROM_ABI void unhandled_exception() noexcept { __op_->__set_exception(std::current_exception()); }

    template <class _Aw>
    _LIBCPP_HIDE_FROM_ABI auto await_transform(_Aw& __a) noexcept {
      return __env_awaiter<_Aw>{__a, __env_};
    }
  };

  coroutine_handle<promise_type> __h_;

  _LIBCPP_HIDE_FROM_ABI __bridge_task() noexcept = default;
  _LIBCPP_HIDE_FROM_ABI __bridge_task(__bridge_task&& __o) noexcept : __h_(std::exchange(__o.__h_, {})) {}
  _LIBCPP_HIDE_FROM_ABI __bridge_task& operator=(__bridge_task&& __o) noexcept {
    if (this != std::addressof(__o)) {
      if (__h_)
        __h_.destroy();
      __h_ = std::exchange(__o.__h_, {});
    }
    return *this;
  }
  _LIBCPP_HIDE_FROM_ABI ~__bridge_task() {
    if (__h_)
      __h_.destroy();
  }

private:
  _LIBCPP_HIDE_FROM_ABI explicit __bridge_task(coroutine_handle<promise_type> __h) noexcept : __h_(__h) {}
};

// P4093R1 section 2 / appendix A (awaitable_sender): wraps an IoAwaitable as a sender. The receiver's environment must
// answer get_io_executor; get_stop_token is used when it answers it.
template <class _IoAw>
class awaitable_sender {
  using __result_t _LIBCPP_NODEBUG    = __bridge_await_result_t<_IoAw>;
  using __value_t _LIBCPP_NODEBUG     = decay_t<__result_t>;
  static constexpr bool __result_void = is_void_v<__result_t>;
  static constexpr bool __result_ec   = !__result_void && __is_ec_outcome_v<__value_t>;

  _IoAw __aw_;

  template <class _Rcvr>
  using __env_t _LIBCPP_NODEBUG = decltype(execution::get_env(std::declval<const _Rcvr&>()));
  template <class _Rcvr>
  using __executor_t _LIBCPP_NODEBUG = decay_t<decltype(get_io_executor(std::declval<__env_t<_Rcvr>>()))>;
  template <class _Rcvr>
  using __token_t _LIBCPP_NODEBUG = remove_cvref_t<decltype(std::get_stop_token(std::declval<__env_t<_Rcvr>>()))>;

  template <class _Rcvr>
  class __op_state {
    using __stored_t _LIBCPP_NODEBUG = conditional_t<__result_void || __result_ec, __as_sender_void, __value_t>;

    _IoAw __aw_;
    _Rcvr __rcvr_;
    __executor_t<_Rcvr> __ex_;
    io_env __env_;
    continuation __start_;
    __stop_bridge<__token_t<_Rcvr>> __stop_;
    __bridge_task<__op_state> __bridge_;
    __as_sender_kind __kind_ = __as_sender_kind::__none;
    optional<__stored_t> __val_;
    error_code __ec_;
    exception_ptr __ep_;

  public:
    using operation_state_concept = execution::operation_state_tag;

    _LIBCPP_HIDE_FROM_ABI __op_state(_IoAw __aw, _Rcvr __rcvr)
        : __aw_(std::move(__aw)), __rcvr_(std::move(__rcvr)), __ex_(get_io_executor(execution::get_env(__rcvr_))) {}
    __op_state(const __op_state&)            = delete;
    __op_state(__op_state&&)                 = delete;
    __op_state& operator=(const __op_state&) = delete;
    __op_state& operator=(__op_state&&)      = delete;

    _LIBCPP_HIDE_FROM_ABI void start() & noexcept {
      __env_.executor                 = executor_ref(__ex_);
      __env_.stop_token               = __stop_.__attach(std::get_stop_token(execution::get_env(__rcvr_)));
      __env_.frame_allocator          = __env_.executor.context().get_frame_allocator();
      __bridge_                       = __run(this);
      __bridge_.__h_.promise().__op_  = this;
      __bridge_.__h_.promise().__env_ = &__env_;
      __start_.h                      = __bridge_.__h_;
      __start_.__chained_             = true; // the frame belongs to this operation state
      __env_.executor.post(__start_);
    }

    // Called by the bridge coroutine.
    _LIBCPP_HIDE_FROM_ABI void __set_exception(exception_ptr __e) noexcept {
      __ep_   = std::move(__e);
      __kind_ = __as_sender_kind::__exception;
    }

    // Called from the final suspension point of the bridge coroutine; nothing may touch the frame afterwards.
    _LIBCPP_HIDE_FROM_ABI void __complete() noexcept {
      switch (__kind_) {
      case __as_sender_kind::__value:
        if constexpr (__result_void || __result_ec)
          execution::set_value(std::move(__rcvr_));
        else
          execution::set_value(std::move(__rcvr_), std::move(*__val_));
        break;
      case __as_sender_kind::__error_code:
        if constexpr (__result_ec) {
          if (__ec_)
            execution::set_error(std::move(__rcvr_), error_code(__ec_));
          else
            execution::set_value(std::move(__rcvr_));
        }
        break;
      case __as_sender_kind::__exception:
        execution::set_error(std::move(__rcvr_), std::move(__ep_));
        break;
      default:
        execution::set_stopped(std::move(__rcvr_));
        break;
      }
    }

  private:
    _LIBCPP_HIDE_FROM_ABI static __bridge_task<__op_state> __run(__op_state* __self) {
      if constexpr (__result_void) {
        co_await __self->__aw_;
        __self->__kind_ =
            __self->__env_.stop_token.stop_requested() ? __as_sender_kind::__stopped : __as_sender_kind::__value;
      } else {
        auto __result = co_await __self->__aw_;
        if (__self->__env_.stop_token.stop_requested()) {
          __self->__kind_ = __as_sender_kind::__stopped;
        } else if constexpr (__result_ec) {
          if constexpr (is_same_v<__value_t, error_code>)
            __self->__ec_ = __result;
          else
            __self->__ec_ = std::get<0>(__result);
          __self->__kind_ = __as_sender_kind::__error_code;
        } else {
          __self->__val_.emplace(std::move(__result));
          __self->__kind_ = __as_sender_kind::__value;
        }
      }
    }
  };

  template <class _Rcvr>
  static constexpr bool __connectable_v = execution::receiver<_Rcvr> && requires(const _Rcvr& __r) {
    { get_io_executor(execution::get_env(__r)) };
    requires(Executor<__executor_t<_Rcvr>> || is_same_v<__executor_t<_Rcvr>, executor_ref>);
  };

public:
  using sender_concept = execution::sender_tag;

  _LIBCPP_HIDE_FROM_ABI explicit awaitable_sender(_IoAw __aw) : __aw_(std::move(__aw)) {}

  template <class _Self, class... _Env>
  _LIBCPP_HIDE_FROM_ABI static consteval auto get_completion_signatures() {
    if constexpr (__result_void)
      return execution::completion_signatures< execution::set_value_t(),
                                               execution::set_error_t(exception_ptr),
                                               execution::set_stopped_t()>{};
    else if constexpr (__result_ec)
      return execution::completion_signatures< execution::set_value_t(),
                                               execution::set_error_t(error_code),
                                               execution::set_error_t(exception_ptr),
                                               execution::set_stopped_t()>{};
    else
      return execution::completion_signatures< execution::set_value_t(__value_t),
                                               execution::set_error_t(exception_ptr),
                                               execution::set_stopped_t()>{};
  }

  template <class _Rcvr>
    requires __connectable_v<_Rcvr>
  _LIBCPP_HIDE_FROM_ABI auto connect(_Rcvr __rcvr) && -> __op_state<_Rcvr> {
    return __op_state<_Rcvr>(std::move(__aw_), std::move(__rcvr));
  }

  template <class _Rcvr>
    requires __connectable_v<_Rcvr> && is_copy_constructible_v<_IoAw>
  _LIBCPP_HIDE_FROM_ABI auto connect(_Rcvr __rcvr) const& -> __op_state<_Rcvr> {
    return __op_state<_Rcvr>(__aw_, std::move(__rcvr));
  }
};

// P4093R1 section 4: the abstraction floor. A result that destructures into (error_code, more...) is not bridged.
template <class _IoAw>
  requires IoAwaitable<remove_cvref_t<_IoAw>>
_LIBCPP_HIDE_FROM_ABI auto as_sender(_IoAw&& __aw) {
  using _Result = decay_t<__bridge_await_result_t<_IoAw>>;
  static_assert(!__is_compound_ec_result_v<_Result>,
                "as_sender does not accept awaitables whose result destructures into (error_code, ...). Wrap the "
                "operation in a task<error_code> that inspects the compound result and returns the error code.");
  return awaitable_sender<remove_cvref_t<_IoAw>>(std::forward<_IoAw>(__aw));
}

} // namespace io

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP_HAS_EXPERIMENTAL_NETWORKING && _LIBCPP_HAS_THREADS

#endif // _LIBCPP___IO_BRIDGE_AS_SENDER_H
