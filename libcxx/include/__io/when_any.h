//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___IO_WHEN_ANY_H
#define _LIBCPP___IO_WHEN_ANY_H

#include <__atomic/memory_order.h>
#include <__config>
#include <__coroutine/coroutine_handle.h>
#include <__cstddef/size_t.h>
#include <__io/continuation.h>
#include <__io/io_awaitable.h>
#include <__io/io_env.h>
#include <__io/join_support.h>
#include <__ranges/concepts.h>
#include <__type_traits/conditional.h>
#include <__type_traits/decay.h>
#include <__type_traits/is_constructible.h>
#include <__type_traits/is_reference.h>
#include <__type_traits/is_void.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/forward.h>
#include <__utility/in_place.h>
#include <__utility/integer_sequence.h>
#include <__utility/move.h>
#include <__utility/pair.h>
#include <__variant/monostate.h>
#include <__vector/vector.h>
#include <optional>
#include <stdexcept>
#include <tuple>
#include <variant>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_EXPERIMENTAL_NETWORKING && _LIBCPP_HAS_THREADS

_LIBCPP_PUSH_MACROS
#  include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

template <class _Aw>
using __any_value_t _LIBCPP_NODEBUG =
    conditional_t<is_void_v<__await_result_t<_Aw>>, monostate, decay_t<__await_result_t<_Aw>>>;

// Awaits child _Ip; if it completes normally and first, it stores its value and requests stop on the others.
template <size_t _Ip, class _Aw, class _Res>
_LIBCPP_HIDE_FROM_ABI __join_runner __any_run(__join_core* __c, _Aw* __a, _Res* __res) {
  if constexpr (is_void_v<__await_result_t<_Aw>>) {
    co_await *__a;
    if (__c->__claim_win()) {
      __res->emplace(in_place_index<_Ip>);
      __c->__stop_.request_stop();
    }
  } else {
    decay_t<__await_result_t<_Aw>> __v = co_await *__a;
    if (__c->__claim_win()) {
      __res->emplace(in_place_index<_Ip>, std::move(__v));
      __c->__stop_.request_stop();
    }
  }
}

// The awaitable of when_any(awaitables...).
template <class... _As>
class [[nodiscard]] __when_any_awaitable {
  static constexpr size_t __n_ = sizeof...(_As);
  using _Result                = variant<__any_value_t<_As>...>;

  tuple<_As...> __as_;
  optional<_Result> __result_;
  __join_core __core_{__n_, false};
  continuation __conts_[__n_];

  template <size_t... _Is>
  _LIBCPP_HIDE_FROM_ABI void __create(index_sequence<_Is...>) {
    ((__conts_[_Is].h = io::__any_run<_Is>(&__core_, &std::get<_Is>(__as_), &__result_).release()), ...);
  }

public:
  _LIBCPP_HIDE_FROM_ABI explicit __when_any_awaitable(_As&&... __as) : __as_(std::move(__as)...) {}
  // Only an awaitable that has not been awaited can be moved.
  _LIBCPP_HIDE_FROM_ABI __when_any_awaitable(__when_any_awaitable&& __o) : __as_(std::move(__o.__as_)) {}
  __when_any_awaitable& operator=(const __when_any_awaitable&) = delete;

  _LIBCPP_HIDE_FROM_ABI bool await_ready() const noexcept { return false; }

  _LIBCPP_HIDE_FROM_ABI void await_suspend(coroutine_handle<> __h, const io_env* __env) {
    __core_.__begin(__h, __env);
    {
      __join_frame_scope __scope(__env);
      __create(index_sequence_for<_As...>{});
    }
    io::__join_post_all(__conts_, __n_, __env->executor);
  }

  // The value of the child that completed first, as the alternative of its position (monostate for void children). If
  // every child exited with an exception, the first of them is rethrown.
  _LIBCPP_HIDE_FROM_ABI _Result await_resume() {
    if (!__core_.__won_.load(memory_order_relaxed))
      std::rethrow_exception(__core_.__first_exception_);
    return std::move(*__result_);
  }
};

// P4100R1 [8.7] (provisional specification: Capy). Awaits all the awaitables concurrently as children of the calling
// coroutine. The first child that completes without an exception wins: stop is requested on the others, the call waits
// until every child has finished (so none outlives it), and then returns variant<R1..Rn> holding the winner's value at
// the winner's position. A child that exits with an exception does not win; if no child wins, the first exception is
// rethrown.
template <class... _As>
  requires(sizeof...(_As) > 0 && (IoAwaitable<decay_t<_As>> && ...) && (is_constructible_v<decay_t<_As>, _As> && ...))
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI auto when_any(_As&&... __as) {
  return __when_any_awaitable<decay_t<_As>...>(decay_t<_As>(std::forward<_As>(__as))...);
}

template <class _Aw>
using __any_range_result_t _LIBCPP_NODEBUG =
    conditional_t<is_void_v<__await_result_t<_Aw>>, size_t, pair<size_t, decay_t<__await_result_t<_Aw>>>>;

template <class _Aw>
_LIBCPP_HIDE_FROM_ABI __join_runner
__any_run_range(__join_core* __c, _Aw* __a, size_t __idx, optional<__any_range_result_t<_Aw>>* __res) {
  if constexpr (is_void_v<__await_result_t<_Aw>>) {
    co_await *__a;
    if (__c->__claim_win()) {
      __res->emplace(__idx);
      __c->__stop_.request_stop();
    }
  } else {
    decay_t<__await_result_t<_Aw>> __v = co_await *__a;
    if (__c->__claim_win()) {
      __res->emplace(__idx, std::move(__v));
      __c->__stop_.request_stop();
    }
  }
}

// The awaitable of when_any(range).
template <class _Aw>
class [[nodiscard]] __when_any_range_awaitable {
  using _Result = __any_range_result_t<_Aw>;

  vector<_Aw> __as_;
  optional<_Result> __result_;
  vector<continuation> __conts_;
  __join_core __core_;

public:
  _LIBCPP_HIDE_FROM_ABI explicit __when_any_range_awaitable(vector<_Aw> __as)
      : __as_(std::move(__as)), __conts_(__as_.size()), __core_(__as_.size(), false) {}
  _LIBCPP_HIDE_FROM_ABI __when_any_range_awaitable(__when_any_range_awaitable&& __o)
      : __when_any_range_awaitable(std::move(__o.__as_)) {}

  _LIBCPP_HIDE_FROM_ABI bool await_ready() const noexcept { return false; }

  _LIBCPP_HIDE_FROM_ABI void await_suspend(coroutine_handle<> __h, const io_env* __env) {
    __core_.__begin(__h, __env);
    {
      __join_frame_scope __scope(__env);
      for (size_t __i = 0; __i != __as_.size(); ++__i)
        __conts_[__i].h = io::__any_run_range<_Aw>(&__core_, &__as_[__i], __i, &__result_).release();
    }
    io::__join_post_all(__conts_.data(), __conts_.size(), __env->executor);
  }

  // The position of the winner (paired with its value unless the children are void). If every child exited with an
  // exception, the first of them is rethrown.
  _LIBCPP_HIDE_FROM_ABI _Result await_resume() {
    if (!__core_.__won_.load(memory_order_relaxed))
      std::rethrow_exception(__core_.__first_exception_);
    return std::move(*__result_);
  }
};

// The range form. Elements are moved out of an rvalue range and copied from an lvalue range. Throws
// invalid_argument if the range is empty.
template <ranges::input_range _Rp>
  requires IoAwaitable<remove_cvref_t<ranges::range_value_t<_Rp>>>
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI auto when_any(_Rp&& __r) {
  using _Aw = remove_cvref_t<ranges::range_value_t<_Rp>>;
  vector<_Aw> __v;
  for (auto&& __e : __r) {
    if constexpr (is_lvalue_reference_v<_Rp>)
      __v.push_back(__e);
    else
      __v.push_back(std::move(__e));
  }
  if (__v.empty())
    std::__throw_invalid_argument("io::when_any requires at least one awaitable");
  return __when_any_range_awaitable<_Aw>(std::move(__v));
}

} // namespace io

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP_HAS_EXPERIMENTAL_NETWORKING && _LIBCPP_HAS_THREADS

#endif // _LIBCPP___IO_WHEN_ANY_H
