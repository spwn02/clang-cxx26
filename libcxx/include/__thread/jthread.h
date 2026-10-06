// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___THREAD_JTHREAD_H
#define _LIBCPP___THREAD_JTHREAD_H

#include <__concepts/same_as.h>
#include <__config>
#include <__stop_token/stop_source.h>
#include <__stop_token/stop_token.h>
#include <__thread/id.h>
#include <__thread/support.h>
#include <__thread/thread.h>
#include <__type_traits/decay.h>
#include <__type_traits/invoke.h>
#include <__type_traits/is_constructible.h>
#include <__type_traits/is_same.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/forward.h>
#include <__utility/integer_sequence.h>
#include <__utility/move.h>
#include <__utility/swap.h>
#include <tuple>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_PUSH_MACROS
#include <__undef_macros>

#if _LIBCPP_STD_VER >= 20 && _LIBCPP_HAS_THREADS

_LIBCPP_BEGIN_NAMESPACE_STD

class jthread {
public:
  // types
  using id                 = thread::id;
  using native_handle_type = thread::native_handle_type;

#  if _LIBCPP_STD_VER >= 26
  template <class _Tp>
  using name_hint       = thread::name_hint<_Tp>;
  using stack_size_hint = thread::stack_size_hint;
#  endif

  // [thread.jthread.cons], constructors, move, and assignment
  _LIBCPP_HIDE_FROM_ABI jthread() noexcept : __stop_source_(std::nostopstate) {}

#  if _LIBCPP_STD_VER < 26
  template <class _Fun, class... _Args>
  _LIBCPP_HIDE_FROM_ABI explicit jthread(_Fun&& __fun, _Args&&... __args)
    requires(!std::is_same_v<remove_cvref_t<_Fun>, jthread>)
      : __stop_source_(),
        __thread_(__init_thread(__stop_source_, std::forward<_Fun>(__fun), std::forward<_Args>(__args)...)) {
    static_assert(is_constructible_v<decay_t<_Fun>, _Fun>);
    static_assert((is_constructible_v<decay_t<_Args>, _Args> && ...));
    static_assert(is_invocable_v<decay_t<_Fun>, decay_t<_Args>...> ||
                  is_invocable_v<decay_t<_Fun>, stop_token, decay_t<_Args>...>);
  }
#  else
  // [thread.jthread.cons]: jthread(attrs..., f, fargs...), see thread
  template <class... _Args>
    requires(sizeof...(_Args) != 0 && !same_as<remove_cvref_t<_Args...[0]>, jthread>)
  _LIBCPP_HIDE_FROM_ABI explicit jthread(_Args&&... __args)
      : __stop_source_(),
        __thread_(__init_thread<__first_non_attribute_index<decay_t<_Args>...>()>(
            __stop_source_,
            __make_index_sequence<__first_non_attribute_index<decay_t<_Args>...>()>(),
            __make_index_sequence<sizeof...(_Args) - __first_non_attribute_index<decay_t<_Args>...>() -
                                  (__first_non_attribute_index<decay_t<_Args>...>() < sizeof...(_Args) ? 1 : 0)>(),
            tuple<_Args&&...>(std::forward<_Args>(__args)...))) {}
#  endif

  _LIBCPP_HIDE_FROM_ABI ~jthread() {
    if (joinable()) {
      request_stop();
      join();
    }
  }

  jthread(const jthread&) = delete;

  _LIBCPP_HIDE_FROM_ABI jthread(jthread&&) noexcept = default;

  jthread& operator=(const jthread&) = delete;

  _LIBCPP_HIDE_FROM_ABI jthread& operator=(jthread&& __other) noexcept {
    if (this != &__other) {
      if (joinable()) {
        request_stop();
        join();
      }
      __stop_source_ = std::move(__other.__stop_source_);
      __thread_      = std::move(__other.__thread_);
    }

    return *this;
  }

  // [thread.jthread.mem], members
  _LIBCPP_HIDE_FROM_ABI void swap(jthread& __other) noexcept {
    std::swap(__stop_source_, __other.__stop_source_);
    std::swap(__thread_, __other.__thread_);
  }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI bool joinable() const noexcept { return get_id() != id(); }

  _LIBCPP_HIDE_FROM_ABI void join() { __thread_.join(); }

  _LIBCPP_HIDE_FROM_ABI void detach() { __thread_.detach(); }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI id get_id() const noexcept { return __thread_.get_id(); }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI native_handle_type native_handle() { return __thread_.native_handle(); }

  // [thread.jthread.stop], stop token handling
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI stop_source get_stop_source() noexcept { return __stop_source_; }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI stop_token get_stop_token() const noexcept { return __stop_source_.get_token(); }

  _LIBCPP_HIDE_FROM_ABI bool request_stop() noexcept { return __stop_source_.request_stop(); }

  // [thread.jthread.special], specialized algorithms
  _LIBCPP_HIDE_FROM_ABI friend void swap(jthread& __lhs, jthread& __rhs) noexcept { __lhs.swap(__rhs); }

  // [thread.jthread.static], static members
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI static unsigned int hardware_concurrency() noexcept {
    return thread::hardware_concurrency();
  }

private:
#  if _LIBCPP_STD_VER >= 26
  template <class... _Ts>
  _LIBCPP_HIDE_FROM_ABI static consteval size_t __first_non_attribute_index() {
    constexpr bool __is_function_arg[] = {!__is_thread_attribute_v<_Ts>..., true};
    size_t __i                     = 0;
    while (!__is_function_arg[__i])
      ++__i;
    return __i;
  }

  template <size_t _I, size_t... _Ai, size_t... _Fi, class... _Ts>
  _LIBCPP_HIDE_FROM_ABI static thread
  __init_thread(const stop_source& __ss, __index_sequence<_Ai...>, __index_sequence<_Fi...>, tuple<_Ts...>&& __all) {
    static_assert(_I < sizeof...(_Ts), "Mandates: a function to invoke follows the thread attributes");
    if constexpr (_I < sizeof...(_Ts)) {
      using _Types = tuple<_Ts...>; // the elements are the (possibly reference) argument types
      using _Fun   = tuple_element_t<_I, _Types>;
      static_assert(is_constructible_v<decay_t<_Fun>, _Fun>);
      static_assert((is_constructible_v<decay_t<tuple_element_t<_I + 1 + _Fi, _Types>>, tuple_element_t<_I + 1 + _Fi, _Types>> &&
                     ...));
      static_assert(is_invocable_v<decay_t<_Fun>, decay_t<tuple_element_t<_I + 1 + _Fi, _Types>>...> ||
                    is_invocable_v<decay_t<_Fun>, stop_token, decay_t<tuple_element_t<_I + 1 + _Fi, _Types>>...>);
      if constexpr (is_invocable_v<decay_t<_Fun>, stop_token, decay_t<tuple_element_t<_I + 1 + _Fi, _Types>>...>) {
        return thread(std::forward<tuple_element_t<_Ai, _Types>>(std::get<_Ai>(std::move(__all)))...,
                      std::forward<_Fun>(std::get<_I>(std::move(__all))),
                      __ss.get_token(),
                      std::forward<tuple_element_t<_I + 1 + _Fi, _Types>>(std::get<_I + 1 + _Fi>(std::move(__all)))...);
      } else {
        return thread(std::forward<tuple_element_t<_Ai, _Types>>(std::get<_Ai>(std::move(__all)))...,
                      std::forward<_Fun>(std::get<_I>(std::move(__all))),
                      std::forward<tuple_element_t<_I + 1 + _Fi, _Types>>(std::get<_I + 1 + _Fi>(std::move(__all)))...);
      }
    } else {
      return thread(); // diagnosed by the static_assert above
    }
  }
#  else
  template <class _Fun, class... _Args>
  _LIBCPP_HIDE_FROM_ABI static thread __init_thread(const stop_source& __ss, _Fun&& __fun, _Args&&... __args) {
    if constexpr (is_invocable_v<decay_t<_Fun>, stop_token, decay_t<_Args>...>) {
      return thread(std::forward<_Fun>(__fun), __ss.get_token(), std::forward<_Args>(__args)...);
    } else {
      return thread(std::forward<_Fun>(__fun), std::forward<_Args>(__args)...);
    }
  }
#  endif

  stop_source __stop_source_;
  thread __thread_;
};

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_STD_VER >= 20 && _LIBCPP_HAS_THREADS

_LIBCPP_POP_MACROS

#endif // _LIBCPP___THREAD_JTHREAD_H
