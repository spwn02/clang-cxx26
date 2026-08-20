// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___STOP_TOKEN_INPLACE_STOP_SOURCE_H
#define _LIBCPP___STOP_TOKEN_INPLACE_STOP_SOURCE_H

#include <__config>
#include <__stop_token/inplace_stop_token.h>
#include <__stop_token/stop_state.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_THREADS

// [stopsource.inplace]
// The non-owning counterpart to `stop_source`: holds the stop state inline (no allocation),
// and unlike `stop_source`, is neither copyable nor movable — `inplace_stop_token`/
// `inplace_stop_callback` refer back to a specific `inplace_stop_source` object by address, so
// that address must stay stable for as long as any such reference is outstanding.
class _LIBCPP_AVAILABILITY_SYNC inplace_stop_source {
public:
  // Deviates from the synopsis (`constexpr inplace_stop_source() noexcept;`): not `constexpr`
  // here, since incrementing `__stop_state`'s atomic counter (needed so `__add_callback` on
  // `__state_` doesn't immediately give up, treating this as a source-less state) is a runtime
  // atomic operation, not a core constant expression.
  _LIBCPP_HIDE_FROM_ABI inplace_stop_source() noexcept { __state_.__increment_stop_source_counter(); }

  inplace_stop_source(const inplace_stop_source&)            = delete;
  inplace_stop_source(inplace_stop_source&&)                 = delete;
  inplace_stop_source& operator=(const inplace_stop_source&) = delete;
  inplace_stop_source& operator=(inplace_stop_source&&)      = delete;
  _LIBCPP_HIDE_FROM_ABI ~inplace_stop_source()                = default;

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI inplace_stop_token get_token() const noexcept {
    return inplace_stop_token(this);
  }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI static constexpr bool stop_possible() noexcept { return true; }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI bool stop_requested() const noexcept { return __state_.__stop_requested(); }

  _LIBCPP_HIDE_FROM_ABI bool request_stop() noexcept { return __state_.__request_stop(); }

private:
  // `mutable`: callback (de)registration mutates the shared atomic state, but is reached
  // through a `const inplace_stop_source*` (the pointer `inplace_stop_token` stores), matching
  // how `stop_source`'s `__intrusive_shared_ptr<__stop_state>` lets the pointee mutate through
  // a `const` path — here the state is stored inline rather than behind a pointer, so the same
  // effect needs an explicit `mutable`.
  mutable __stop_state __state_;

  friend class inplace_stop_token;
  template <class>
  friend class inplace_stop_callback;
};

_LIBCPP_HIDE_FROM_ABI inline bool inplace_stop_token::stop_requested() const noexcept {
  return __source_ != nullptr && __source_->stop_requested();
}

#endif // _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_THREADS

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___STOP_TOKEN_INPLACE_STOP_SOURCE_H
