//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___EXECUTION_WITH_AWAITABLE_SENDERS_H
#define _LIBCPP___EXECUTION_WITH_AWAITABLE_SENDERS_H

#include <__concepts/same_as.h>
#include <__config>
#include <__coroutine/coroutine_handle.h>
#include <__execution/as_awaitable.h>
#include <__utility/forward.h>
#include <exception>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

namespace execution {

// [exec.with.awaitable.senders]: with_awaitable_senders. Used as the base class of a
// coroutine promise type to make senders awaitable in that coroutine, and to give a default
// unhandled_stopped() that terminates unless a downstream continuation overrides it.
//
// The exposition-only private members `continuation`/`stopped-handler` are spelled here as
// `__continuation_`/`__stopped_handler_` -- the standard's own text reuses the unadorned name
// `continuation` for both the private data member and the public accessor method, which is
// exposition shorthand, not literal C++ (a real implementation needs two distinct names).
template <class _Promise>
struct with_awaitable_senders {
  template <class _OtherPromise>
    requires(!same_as<_OtherPromise, void>)
  _LIBCPP_HIDE_FROM_ABI void set_continuation(coroutine_handle<_OtherPromise> __h) noexcept {
    __continuation_ = __h;
    if constexpr (requires(_OtherPromise& __other) { __other.unhandled_stopped(); }) {
      __stopped_handler_ = [](void* __p) noexcept -> coroutine_handle<> {
        return coroutine_handle<_OtherPromise>::from_address(__p).promise().unhandled_stopped();
      };
    } else {
      __stopped_handler_ = &__default_unhandled_stopped;
    }
  }

  _LIBCPP_HIDE_FROM_ABI coroutine_handle<> continuation() const noexcept { return __continuation_; }

  _LIBCPP_HIDE_FROM_ABI coroutine_handle<> unhandled_stopped() noexcept {
    return __stopped_handler_(__continuation_.address());
  }

  template <class _Value>
  _LIBCPP_HIDE_FROM_ABI decltype(auto) await_transform(_Value&& __value) {
    return execution::as_awaitable(std::forward<_Value>(__value), static_cast<_Promise&>(*this));
  }

private:
  [[noreturn]] _LIBCPP_HIDE_FROM_ABI static coroutine_handle<> __default_unhandled_stopped(void*) noexcept {
    std::terminate();
  }

  coroutine_handle<> __continuation_{};
  coroutine_handle<> (*__stopped_handler_)(void*) noexcept = &__default_unhandled_stopped;
};

} // namespace execution

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___EXECUTION_WITH_AWAITABLE_SENDERS_H
