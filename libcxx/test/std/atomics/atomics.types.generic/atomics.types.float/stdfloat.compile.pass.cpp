//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// <atomic>

// Extended floating-point atomics participate in the floating-point specialization.

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

#include <atomic>
#include <concepts>
#include <stdfloat>

template <class _Tp>
concept HasFloatingFetchAdd = requires(std::atomic<_Tp>& __value, std::atomic_ref<_Tp> __ref) {
  { __value.fetch_add(_Tp(1)) } -> std::same_as<_Tp>;
  { __value.fetch_sub(_Tp(1)) } -> std::same_as<_Tp>;
  { __ref.fetch_add(_Tp(1)) } -> std::same_as<_Tp>;
  { __ref.fetch_sub(_Tp(1)) } -> std::same_as<_Tp>;
};

#ifdef __STDCPP_FLOAT16_T__
static_assert(HasFloatingFetchAdd<std::float16_t>);
#endif
#ifdef __STDCPP_FLOAT32_T__
static_assert(HasFloatingFetchAdd<std::float32_t>);
#endif
#ifdef __STDCPP_FLOAT64_T__
static_assert(HasFloatingFetchAdd<std::float64_t>);
#endif
#ifdef __STDCPP_BFLOAT16_T__
static_assert(HasFloatingFetchAdd<std::bfloat16_t>);
#endif

