// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___IO_BRIDGE_AWAIT_SENDER_H
#define _LIBCPP___IO_BRIDGE_AWAIT_SENDER_H

#include <__config>
#include <__coroutine/coroutine_handle.h>
#include <__exception/exception_ptr.h>
#include <__execution/completion_functions.h>
#include <__execution/completion_signatures.h>
#include <__execution/connect.h>
#include <__execution/get_completion_signatures.h>
#include <__execution/get_env.h>
#include <__execution/get_stop_token.h>
#include <__execution/operation_state.h>
#include <__execution/receiver.h>
#include <__execution/sender.h>
#include <__io/continuation.h>
#include <__io/io_awaitable.h>
#include <__io/io_env.h>
#include <__io_stream/error.h>
#include <__memory/construct_at.h>
#include <__new/launder.h>
#include <__stop_token/stop_token.h>
#include <__system_error/error_code.h>
#include <__system_error/system_error.h>
#include <__type_traits/conditional.h>
#include <__type_traits/decay.h>
#include <__type_traits/is_constructible.h>
#include <__type_traits/is_nothrow_constructible.h>
#include <__type_traits/is_same.h>
#include <__type_traits/is_void.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/declval.h>
#include <__utility/forward.h>
#include <__utility/move.h>
#include <__verbose_abort>
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

// P4092R1 appendix A (bridge_env): the environment the awaited sender is connected with. It answers get_stop_token with
// the stop token of the awaiting coroutine's io_env. It points at the token instead of copying it.
struct __bridge_env {
  const stop_token* __st_;
  _LIBCPP_HIDE_FROM_ABI stop_token query(std::get_stop_token_t) const noexcept { return *__st_; }
};

// P4092R1 section 4: the stopped completion is surfaced as an exception when the result is not an io_result. The paper
// leaves the type open ("a production implementation would use a project-appropriate cancellation exception"): this is
// a system_error that carries io::error::canceled, so that catching it and comparing against io::cond::canceled works
// the same way as inspecting the error_code of the io_result path.
[[noreturn]] _LIBCPP_HIDE_FROM_ABI inline void __throw_operation_canceled() {
#  if _LIBCPP_HAS_EXCEPTIONS
  throw system_error(make_error_code(error::canceled));
#  else
  _LIBCPP_VERBOSE_ABORT("await_sender: the sender completed with set_stopped and exceptions are disabled");
#  endif
}

template <class... _Ts>
struct __bridge_list {};

template <class _List>
inline constexpr bool __has_error_code_v = false;
template <class... _Es>
inline constexpr bool __has_error_code_v<__bridge_list<_Es...>> = (is_same_v<remove_cvref_t<_Es>, error_code> || ...);

template <class _Sndr>
inline constexpr bool __sender_has_error_code_v =
    __has_error_code_v<execution::error_types_of_t<_Sndr, __bridge_env, __bridge_list>>;

enum class __bridge_kind : unsigned char { __none, __value, __error_code, __exception, __stopped };

struct __bridge_void {};

// P4092R1 section 4 / appendix A (sender_awaitable). The operation state lives inside the awaitable (and the awaitable
// inside the awaiting coroutine's frame), so there is no allocation beyond that frame. The sender completes on whatever
// thread it likes; the result is stored and the coroutine is posted to the executor of its io_env.
template <class _Sndr>
class sender_awaitable {
  using __value_t _LIBCPP_NODEBUG = execution::__single_sender_value_type<_Sndr, __bridge_env>;
  static constexpr bool __has_ec  = __sender_has_error_code_v<_Sndr>;
  using __stored_t _LIBCPP_NODEBUG = conditional_t<is_void_v<__value_t>, __bridge_void, __value_t>;

  struct __receiver {
    using receiver_concept = execution::receiver_tag;
    sender_awaitable* __self_;

    _LIBCPP_HIDE_FROM_ABI __bridge_env get_env() const noexcept { return __bridge_env{&__self_->__env_->stop_token}; }

    template <class... _Args>
    _LIBCPP_HIDE_FROM_ABI void set_value(_Args&&... __args) && noexcept {
#  if _LIBCPP_HAS_EXCEPTIONS
      try {
#  endif
        if constexpr (is_void_v<__value_t>)
          __self_->__val_.emplace();
        else
          __self_->__val_.emplace(std::forward<_Args>(__args)...);
        __self_->__kind_ = __bridge_kind::__value;
#  if _LIBCPP_HAS_EXCEPTIONS
      } catch (...) {
        __self_->__ep_   = std::current_exception();
        __self_->__kind_ = __bridge_kind::__exception;
      }
#  endif
      __self_->__resume_on_executor();
    }

    template <class _Err>
    _LIBCPP_HIDE_FROM_ABI void set_error(_Err&& __e) && noexcept {
      if constexpr (__has_ec && is_same_v<decay_t<_Err>, error_code>) {
        __self_->__ec_   = __e;
        __self_->__kind_ = __bridge_kind::__error_code;
      } else {
#  if _LIBCPP_HAS_EXCEPTIONS
        try {
#  endif
          __self_->__ep_ = execution::__as_except_ptr(std::forward<_Err>(__e));
#  if _LIBCPP_HAS_EXCEPTIONS
        } catch (...) {
          __self_->__ep_ = std::current_exception();
        }
#  endif
        __self_->__kind_ = __bridge_kind::__exception;
      }
      __self_->__resume_on_executor();
    }

    _LIBCPP_HIDE_FROM_ABI void set_stopped() && noexcept {
      __self_->__kind_ = __bridge_kind::__stopped;
      __self_->__resume_on_executor();
    }
  };

  using __op_t _LIBCPP_NODEBUG = decltype(execution::connect(std::declval<_Sndr>(), std::declval<__receiver>()));

  _Sndr __sndr_;
  const io_env* __env_ = nullptr;
  continuation __cont_;
  __bridge_kind __kind_ = __bridge_kind::__none;
  optional<__stored_t> __val_;
  error_code __ec_;
  exception_ptr __ep_;
  alignas(__op_t) unsigned char __op_buf_[sizeof(__op_t)];
  bool __op_constructed_ = false;

  _LIBCPP_HIDE_FROM_ABI void __resume_on_executor() noexcept { __env_->executor.post(__cont_); }

  _LIBCPP_HIDE_FROM_ABI __op_t* __op() noexcept { return std::launder(reinterpret_cast<__op_t*>(__op_buf_)); }

  // The sender can complete with set_error(error_code): the result is an io_result, the error_code is a value.
  _LIBCPP_HIDE_FROM_ABI auto __resume_ec() {
    if (__kind_ == __bridge_kind::__exception)
      std::rethrow_exception(std::move(__ep_));
    error_code __code;
    if (__kind_ == __bridge_kind::__stopped)
      __code = make_error_code(error::canceled);
    else if (__kind_ == __bridge_kind::__error_code)
      __code = __ec_;
    if constexpr (is_void_v<__value_t>) {
      return io_result<>{__code};
    } else {
      // A completion with several values keeps them together as one tuple element (P4092R1 appendix A).
      if (__kind_ == __bridge_kind::__value)
        return io_result<__value_t>{error_code{}, std::move(*__val_)};
      return io_result<__value_t>{__code, __value_t{}};
    }
  }

  // The sender cannot complete with set_error(error_code): return the value, rethrow exceptions, and surface the
  // stopped completion as an exception.
  _LIBCPP_HIDE_FROM_ABI __value_t __resume_plain() {
    if (__kind_ == __bridge_kind::__exception)
      std::rethrow_exception(std::move(__ep_));
    if (__kind_ == __bridge_kind::__stopped)
      io::__throw_operation_canceled();
    if constexpr (!is_void_v<__value_t>)
      return std::move(*__val_);
  }

public:
  _LIBCPP_HIDE_FROM_ABI explicit sender_awaitable(_Sndr __s) : __sndr_(std::move(__s)) {}
  sender_awaitable(const sender_awaitable&)            = delete;
  sender_awaitable& operator=(const sender_awaitable&) = delete;
  sender_awaitable& operator=(sender_awaitable&&)      = delete;
  // Movable only before the awaitable is started.
  _LIBCPP_HIDE_FROM_ABI sender_awaitable(sender_awaitable&& __o) noexcept(is_nothrow_move_constructible_v<_Sndr>)
      : __sndr_(std::move(__o.__sndr_)) {}

  _LIBCPP_HIDE_FROM_ABI ~sender_awaitable() {
    if (__op_constructed_)
      std::__destroy_at(__op());
  }

  _LIBCPP_HIDE_FROM_ABI bool await_ready() const noexcept { return false; }

  // P4003R3 4.2: the second argument is the environment of the awaiting coroutine. The operation state is constructed
  // in place and started; the receiver posts the coroutine to env->executor when the sender completes. Nothing here
  // touches *this after start(), because the completion may run (and resume the coroutine) concurrently.
  _LIBCPP_HIDE_FROM_ABI void await_suspend(coroutine_handle<> __h, const io_env* __env) {
    __env_    = __env;
    __cont_.h = __h;
    ::new (static_cast<void*>(__op_buf_)) __op_t(execution::connect(std::move(__sndr_), __receiver{this}));
    __op_constructed_ = true;
    execution::start(*__op());
  }

  _LIBCPP_HIDE_FROM_ABI auto await_resume() {
    if constexpr (__has_ec)
      return __resume_ec();
    else
      return __resume_plain();
  }
};

// P4092R1 section 3: co_await await_sender(sndr) from any coroutine that propagates its io_env. Requires a sender with
// at most one set_value shape.
template <class _Sndr>
  requires execution::sender_in<remove_cvref_t<_Sndr>, __bridge_env> &&
           requires { typename execution::__single_sender_value_type<remove_cvref_t<_Sndr>, __bridge_env>; }
_LIBCPP_HIDE_FROM_ABI auto await_sender(_Sndr&& __sndr) {
  return sender_awaitable<remove_cvref_t<_Sndr>>(std::forward<_Sndr>(__sndr));
}

} // namespace io

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP_HAS_EXPERIMENTAL_NETWORKING && _LIBCPP_HAS_THREADS

#endif // _LIBCPP___IO_BRIDGE_AWAIT_SENDER_H
