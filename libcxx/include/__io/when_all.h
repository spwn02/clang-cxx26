//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___IO_WHEN_ALL_H
#define _LIBCPP___IO_WHEN_ALL_H

#include <__atomic/memory_order.h>
#include <__config>
#include <__coroutine/coroutine_handle.h>
#include <__cstddef/size_t.h>
#include <__io/continuation.h>
#include <__io/io_awaitable.h>
#include <__io/io_env.h>
#include <__io/join_support.h>
#include <__memory/addressof.h>
#include <__ranges/concepts.h>
#include <__type_traits/conditional.h>
#include <__type_traits/decay.h>
#include <__type_traits/is_constructible.h>
#include <__type_traits/is_reference.h>
#include <__type_traits/is_void.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/forward.h>
#include <__utility/integer_sequence.h>
#include <__utility/move.h>
#include <__vector/vector.h>
#include <optional>
#include <stdexcept>
#include <tuple>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_EXPERIMENTAL_NETWORKING && _LIBCPP_HAS_THREADS

_LIBCPP_PUSH_MACROS
#  include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

struct __join_void {};

template <class _Aw>
using __join_slot_t _LIBCPP_NODEBUG =
    conditional_t<is_void_v<__await_result_t<_Aw>>, __join_void, optional<decay_t<__await_result_t<_Aw>>>>;

template <class... _As>
inline constexpr bool __join_has_values = ((!is_void_v<__await_result_t<_As>>) || ...);

// Awaits one child and parks its value.
template <class _Aw, class _Slot>
_LIBCPP_HIDE_FROM_ABI __join_runner __all_run(__join_core*, _Aw* __a, _Slot* __s) {
  if constexpr (is_void_v<__await_result_t<_Aw>>)
    co_await *__a;
  else
    __s->emplace(co_await *__a);
}

// The awaitable of when_all(awaitables...).
template <class... _As>
class [[nodiscard]] __when_all_awaitable {
  static constexpr size_t __n_ = sizeof...(_As);

  tuple<_As...> __as_;
  tuple<__join_slot_t<_As>...> __slots_;
  __join_core __core_{__n_, true};
  continuation __conts_[__n_];

  template <size_t _Ip>
  _LIBCPP_HIDE_FROM_ABI auto __take() {
    using _Aw = tuple_element_t<_Ip, tuple<_As...>>;
    if constexpr (is_void_v<__await_result_t<_Aw>>)
      return tuple<>();
    else
      return tuple<decay_t<__await_result_t<_Aw>>>(std::move(*std::get<_Ip>(__slots_)));
  }

  template <size_t... _Is>
  _LIBCPP_HIDE_FROM_ABI auto __results(index_sequence<_Is...>) {
    return std::tuple_cat(__take<_Is>()...);
  }

  template <size_t... _Is>
  _LIBCPP_HIDE_FROM_ABI void __create(index_sequence<_Is...>) {
    ((__conts_[_Is].h =
          io::__all_run(
              std::addressof(__core_), std::addressof(std::get<_Is>(__as_)), std::addressof(std::get<_Is>(__slots_)))
              .release()),
     ...);
  }

public:
  _LIBCPP_HIDE_FROM_ABI explicit __when_all_awaitable(_As&&... __as) : __as_(std::move(__as)...) {}
  // Only an awaitable that has not been awaited can be moved.
  _LIBCPP_HIDE_FROM_ABI __when_all_awaitable(__when_all_awaitable&& __o) : __as_(std::move(__o.__as_)) {}
  __when_all_awaitable& operator=(const __when_all_awaitable&) = delete;

  _LIBCPP_HIDE_FROM_ABI bool await_ready() const noexcept { return false; }

  _LIBCPP_HIDE_FROM_ABI void await_suspend(coroutine_handle<> __h, const io_env* __env) {
    __core_.__begin(__h, __env);
    {
      __join_frame_scope __scope(__env);
      __create(index_sequence_for<_As...>{});
    }
    io::__join_post_all(__conts_, __n_, __env->executor);
  }

  // Rethrows the first exception of a child; otherwise the tuple of the values of the children that return one, in
  // argument order (void when no child returns a value).
  _LIBCPP_HIDE_FROM_ABI auto await_resume() {
    if (__core_.__failed_.load(memory_order_relaxed))
      std::rethrow_exception(__core_.__first_exception_);
    if constexpr (__join_has_values<_As...>)
      return __results(index_sequence_for<_As...>{});
  }
};

// P4100R1 [8.7] (provisional specification: Capy). Awaits all the awaitables concurrently as children of the calling
// coroutine and completes when every one has completed. The children are started in argument order through the caller's
// executor, with the caller's frame allocator and a stop token that fires when the caller's token fires or when a child
// fails. If a child exits with an exception, stop is requested on the others and, once all have finished, the first
// exception is rethrown. Otherwise the result is a tuple of the values of the children that return one.
template <class... _As>
  requires(sizeof...(_As) > 0 && (IoAwaitable<decay_t<_As>> && ...) && (is_constructible_v<decay_t<_As>, _As> && ...))
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI auto when_all(_As&&... __as) {
  return __when_all_awaitable<decay_t<_As>...>(decay_t<_As>(std::forward<_As>(__as))...);
}

// The awaitable of when_all(range).
template <class _Aw>
class [[nodiscard]] __when_all_range_awaitable {
  using _Value _LIBCPP_NODEBUG = __await_result_t<_Aw>;

  vector<_Aw> __as_;
  vector<__join_slot_t<_Aw>> __slots_;
  vector<continuation> __conts_;
  __join_core __core_;

public:
  _LIBCPP_HIDE_FROM_ABI explicit __when_all_range_awaitable(vector<_Aw> __as)
      : __as_(std::move(__as)), __slots_(__as_.size()), __conts_(__as_.size()), __core_(__as_.size(), true) {}
  _LIBCPP_HIDE_FROM_ABI __when_all_range_awaitable(__when_all_range_awaitable&& __o)
      : __when_all_range_awaitable(std::move(__o.__as_)) {}

  _LIBCPP_HIDE_FROM_ABI bool await_ready() const noexcept { return false; }

  _LIBCPP_HIDE_FROM_ABI void await_suspend(coroutine_handle<> __h, const io_env* __env) {
    __core_.__begin(__h, __env);
    {
      __join_frame_scope __scope(__env);
      for (size_t __i = 0; __i != __as_.size(); ++__i)
        __conts_[__i].h =
            io::__all_run(std::addressof(__core_), std::addressof(__as_[__i]), std::addressof(__slots_[__i])).release();
    }
    io::__join_post_all(__conts_.data(), __conts_.size(), __env->executor);
  }

  // Rethrows the first exception of a child; otherwise a vector with the values in input order (void for void
  // children).
  _LIBCPP_HIDE_FROM_ABI auto await_resume() {
    if (__core_.__failed_.load(memory_order_relaxed))
      std::rethrow_exception(__core_.__first_exception_);
    if constexpr (!is_void_v<_Value>) {
      vector<decay_t<_Value>> __r;
      __r.reserve(__slots_.size());
      for (auto& __s : __slots_)
        __r.push_back(std::move(*__s));
      return __r;
    }
  }
};

// The range form. Elements are moved out of an rvalue range and copied from an lvalue range. Throws
// invalid_argument if the range is empty.
template <ranges::input_range _Rp>
  requires IoAwaitable<remove_cvref_t<ranges::range_value_t<_Rp>>>
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI auto when_all(_Rp&& __r) {
  using _Aw = remove_cvref_t<ranges::range_value_t<_Rp>>;
  vector<_Aw> __v;
  for (auto&& __e : __r) {
    if constexpr (is_lvalue_reference_v<_Rp>)
      __v.push_back(__e);
    else
      __v.push_back(std::move(__e));
  }
  if (__v.empty())
    std::__throw_invalid_argument("io::when_all requires at least one awaitable");
  return __when_all_range_awaitable<_Aw>(std::move(__v));
}

} // namespace io

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP_HAS_EXPERIMENTAL_NETWORKING && _LIBCPP_HAS_THREADS

#endif // _LIBCPP___IO_WHEN_ALL_H
