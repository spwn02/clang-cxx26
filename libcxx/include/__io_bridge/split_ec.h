// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___IO_BRIDGE_SPLIT_EC_H
#define _LIBCPP___IO_BRIDGE_SPLIT_EC_H

#include <__config>
#include <__exception/exception_ptr.h>
#include <__execution/completion_signatures.h>
#include <__execution/connect.h>
#include <__execution/get_completion_signatures.h>
#include <__execution/get_env.h>
#include <__execution/operation_state.h>
#include <__execution/receiver.h>
#include <__execution/sender.h>
#include <__execution/sender_adaptor_closure.h>
#include <__system_error/error_code.h>
#include <__type_traits/conditional.h>
#include <__type_traits/is_constructible.h>
#include <__type_traits/is_nothrow_constructible.h>
#include <__type_traits/is_same.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/declval.h>
#include <__utility/forward.h>
#include <__utility/move.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_EXPERIMENTAL_NETWORKING && _LIBCPP_HAS_THREADS

_LIBCPP_PUSH_MACROS
#  include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

template <class... _Fns>
struct __split_sigs_list {};

// Appends the signatures of _Rhs that _Lhs does not hold yet.
template <class _Lhs, class... _Bs>
struct __split_sigs_add {
  using type = _Lhs;
};
template <class... _As, class _Rhs, class... _Bs>
struct __split_sigs_add<__split_sigs_list<_As...>, _Rhs, _Bs...>
    : __split_sigs_add<conditional_t<(is_same_v<_As, _Rhs> || ...), __split_sigs_list<_As...>, __split_sigs_list<_As..., _Rhs>>,
                       _Bs...> {};

template <class _Lhs, class _Rhs>
struct __split_sigs_cat;
template <class _Lhs, class... _Bs>
struct __split_sigs_cat<_Lhs, __split_sigs_list<_Bs...>> : __split_sigs_add<_Lhs, _Bs...> {};

// set_value(error_code) becomes set_value() and set_error(error_code); everything else passes through. A value
// completion of any other shape cannot be split.
template <class _Fn>
struct __split_one {
  using type = __split_sigs_list<_Fn>;
};
template <>
struct __split_one<execution::set_value_t(error_code)> {
  using type = __split_sigs_list<execution::set_value_t(), execution::set_error_t(error_code)>;
};
template <>
struct __split_one<execution::set_value_t(error_code&)> : __split_one<execution::set_value_t(error_code)> {};
template <>
struct __split_one<execution::set_value_t(const error_code&)> : __split_one<execution::set_value_t(error_code)> {};
template <>
struct __split_one<execution::set_value_t(error_code&&)> : __split_one<execution::set_value_t(error_code)> {};
template <class... _Args>
struct __split_one<execution::set_value_t(_Args...)> {
  static_assert(sizeof...(_Args) == 0, "split_ec requires every value completion of the sender to be set_value() or "
                                       "set_value(std::error_code)");
  using type = __split_sigs_list<execution::set_value_t()>;
};

template <class _Sigs>
struct __split_sigs;
template <class... _Fns>
struct __split_sigs<execution::completion_signatures<_Fns...>> {
  template <class _Acc, class... _Rest>
  struct __fold {
    using type = _Acc;
  };
  template <class _Acc, class _First, class... _Rest>
  struct __fold<_Acc, _First, _Rest...> : __fold<typename __split_sigs_cat<_Acc, typename __split_one<_First>::type>::type, _Rest...> {};
  template <class... _Out>
  static execution::completion_signatures<_Out...> __to_sigs(__split_sigs_list<_Out...>);
  using type = decltype(__to_sigs(std::declval<typename __fold<__split_sigs_list<>, _Fns...>::type>()));
};

template <class _Rcvr>
class __split_ec_receiver {
  _Rcvr __rcvr_;

public:
  using receiver_concept = execution::receiver_tag;

  _LIBCPP_HIDE_FROM_ABI explicit __split_ec_receiver(_Rcvr __r) noexcept(is_nothrow_move_constructible_v<_Rcvr>)
      : __rcvr_(std::move(__r)) {}

  _LIBCPP_HIDE_FROM_ABI decltype(auto) get_env() const noexcept { return execution::get_env(__rcvr_); }

  _LIBCPP_HIDE_FROM_ABI void set_value(error_code __ec) && noexcept {
    if (!__ec)
      execution::set_value(std::move(__rcvr_));
    else
      execution::set_error(std::move(__rcvr_), std::move(__ec));
  }
  _LIBCPP_HIDE_FROM_ABI void set_value() && noexcept { execution::set_value(std::move(__rcvr_)); }
  template <class _Err>
  _LIBCPP_HIDE_FROM_ABI void set_error(_Err&& __e) && noexcept {
    execution::set_error(std::move(__rcvr_), std::forward<_Err>(__e));
  }
  _LIBCPP_HIDE_FROM_ABI void set_stopped() && noexcept { execution::set_stopped(std::move(__rcvr_)); }
};

// P4093R1 section 8 / appendix A: a receiver adapter, no type erasure, no allocation.
template <class _Sndr>
class __split_ec_sender {
  _Sndr __sndr_;

public:
  using sender_concept = execution::sender_tag;

  _LIBCPP_HIDE_FROM_ABI explicit __split_ec_sender(_Sndr __s) : __sndr_(std::move(__s)) {}

  template <class _Self, class... _Env>
    requires execution::sender_in<_Sndr, _Env...>
  _LIBCPP_HIDE_FROM_ABI static consteval auto get_completion_signatures() {
    return typename __split_sigs<execution::completion_signatures_of_t<_Sndr, _Env...>>::type{};
  }

  template <class _Rcvr>
    requires execution::receiver<_Rcvr> && execution::__sender_to<_Sndr, __split_ec_receiver<remove_cvref_t<_Rcvr>>>
  _LIBCPP_HIDE_FROM_ABI auto connect(_Rcvr&& __rcvr) && {
    return execution::connect(std::move(__sndr_), __split_ec_receiver<remove_cvref_t<_Rcvr>>(std::forward<_Rcvr>(__rcvr)));
  }
  template <class _Rcvr>
    requires execution::receiver<_Rcvr> && is_copy_constructible_v<_Sndr> &&
             execution::__sender_to<const _Sndr&, __split_ec_receiver<remove_cvref_t<_Rcvr>>>
  _LIBCPP_HIDE_FROM_ABI auto connect(_Rcvr&& __rcvr) const& {
    return execution::connect(__sndr_, __split_ec_receiver<remove_cvref_t<_Rcvr>>(std::forward<_Rcvr>(__rcvr)));
  }
};

struct __split_ec_closure : execution::sender_adaptor_closure<__split_ec_closure> {
  template <execution::sender _Sndr>
  _LIBCPP_HIDE_FROM_ABI auto operator()(_Sndr&& __sndr) const {
    return __split_ec_sender<remove_cvref_t<_Sndr>>(std::forward<_Sndr>(__sndr));
  }
};

// split_ec(sndr) and sndr | split_ec(): turns a sender that completes with set_value(error_code) into one that completes
// with set_value() when the code is zero and with set_error(ec) otherwise, so that the error channel carries the failure
// (upon_error reaches it, when_all cancels its siblings).
struct split_ec_t {
  template <execution::sender _Sndr>
  _LIBCPP_HIDE_FROM_ABI auto operator()(_Sndr&& __sndr) const {
    return __split_ec_sender<remove_cvref_t<_Sndr>>(std::forward<_Sndr>(__sndr));
  }
  _LIBCPP_HIDE_FROM_ABI __split_ec_closure operator()() const noexcept { return {}; }
};
inline constexpr split_ec_t split_ec{};

} // namespace io

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP_HAS_EXPERIMENTAL_NETWORKING && _LIBCPP_HAS_THREADS

#endif // _LIBCPP___IO_BRIDGE_SPLIT_EC_H
