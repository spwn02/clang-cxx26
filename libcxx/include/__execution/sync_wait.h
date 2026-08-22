//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___EXECUTION_SYNC_WAIT_H
#define _LIBCPP___EXECUTION_SYNC_WAIT_H

#include <__config>
#include <__execution/completion_functions.h>
#include <__execution/connect.h>
#include <__execution/domain.h>
#include <__execution/get_completion_signatures.h>
#include <__execution/get_scheduler.h>
#include <__execution/operation_state.h>
#include <__execution/queryable.h>
#include <__execution/receiver.h>
#include <__execution/run_loop.h>
#include <__execution/sender.h>
#include <__type_traits/decay.h>
#include <__type_traits/is_same.h>
#include <__type_traits/remove_cvref.h>
#include <__type_traits/type_identity.h>
#include <__utility/forward.h>
#include <__utility/move.h>
#include <exception>
#include <optional>
#include <system_error>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_THREADS

namespace this_thread {

// [exec.sync.wait]p2: sync-wait-env.
struct __sync_wait_env {
  execution::run_loop* __loop;

  _LIBCPP_HIDE_FROM_ABI constexpr auto query(execution::get_scheduler_t) const noexcept {
    return __loop->get_scheduler();
  }
  _LIBCPP_HIDE_FROM_ABI constexpr auto query(execution::get_start_scheduler_t) const noexcept {
    return __loop->get_scheduler();
  }
  _LIBCPP_HIDE_FROM_ABI constexpr auto query(execution::get_delegation_scheduler_t) const noexcept {
    return __loop->get_scheduler();
  }
};

// [exec.sync.wait]p3: sync-wait-result-type.
template <execution::sender_in<__sync_wait_env> _Sndr>
using __sync_wait_result_type =
    optional<execution::value_types_of_t<_Sndr, __sync_wait_env, execution::__decayed_tuple, type_identity_t>>;

template <class _Sndr>
struct __sync_wait_state {
  execution::run_loop __loop;
  exception_ptr __error;
  __sync_wait_result_type<_Sndr> __result;
};

// [exec.sync.wait]p6-9: sync-wait-receiver.
template <class _Sndr>
class __sync_wait_receiver {
public:
  using receiver_concept = execution::receiver_tag;

  _LIBCPP_HIDE_FROM_ABI constexpr explicit __sync_wait_receiver(__sync_wait_state<_Sndr>* __state) noexcept
      : __state_(__state) {}

  template <class... _Args>
  _LIBCPP_HIDE_FROM_ABI constexpr void set_value(_Args&&... __args) && noexcept {
    try {
      __state_->__result.emplace(std::forward<_Args>(__args)...);
    } catch (...) {
      __state_->__error = current_exception();
    }
    __state_->__loop.finish();
  }

  template <class _Error>
  _LIBCPP_HIDE_FROM_ABI constexpr void set_error(_Error&& __err) && noexcept {
    __state_->__error = execution::__as_except_ptr(std::forward<_Error>(__err));
    __state_->__loop.finish();
  }

  _LIBCPP_HIDE_FROM_ABI constexpr void set_stopped() && noexcept { __state_->__loop.finish(); }

  _LIBCPP_HIDE_FROM_ABI constexpr __sync_wait_env get_env() const noexcept {
    return __sync_wait_env{&__state_->__loop};
  }

private:
  __sync_wait_state<_Sndr>* __state_;
};

// [exec.sync.wait]p4-5,10-11.
struct sync_wait_t {
  template <class _Sndr>
    requires execution::sender_in<_Sndr, __sync_wait_env>
  _LIBCPP_HIDE_FROM_ABI auto operator()(_Sndr&& __sndr) const -> __sync_wait_result_type<remove_cvref_t<_Sndr>> {
    return execution::apply_sender(
        execution::__completion_domain(__sndr, __sync_wait_env{nullptr}), *this, std::forward<_Sndr>(__sndr));
  }

  // [exec.sync.wait]p10: `sync_wait.apply_sender(sndr)` -- reached via default_domain's
  // fallback dispatch (<__execution/domain.h>), since nothing in scope through M4 provides a
  // completion domain that would override it.
  template <class _Sndr>
  _LIBCPP_HIDE_FROM_ABI auto apply_sender(_Sndr&& __sndr) const -> __sync_wait_result_type<remove_cvref_t<_Sndr>> {
    __sync_wait_state<remove_cvref_t<_Sndr>> __state;
    auto __op = execution::connect(std::forward<_Sndr>(__sndr), __sync_wait_receiver<remove_cvref_t<_Sndr>>(&__state));
    execution::start(__op);
    __state.__loop.run();
    if (__state.__error) {
      rethrow_exception(std::move(__state.__error));
    }
    return std::move(__state.__result);
  }
};

inline constexpr sync_wait_t sync_wait{};

// this_thread::sync_wait_with_variant is not implemented: [exec.sync.wait.var]p3's
// `apply_sender` is specified directly in terms of `into_variant` (`sync_wait(into_variant
// (sndr))`), an M5 sender adaptor (docs/CXX26_GAPS.md) not yet built. Revisit once M5 lands
// into_variant.

} // namespace this_thread

#endif // _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_THREADS

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___EXECUTION_SYNC_WAIT_H
