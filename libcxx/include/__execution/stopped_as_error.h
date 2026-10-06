//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___EXECUTION_STOPPED_AS_ERROR_H
#define _LIBCPP___EXECUTION_STOPPED_AS_ERROR_H

#include <__concepts/constructible.h>
#include <__config>
#include <__execution/completion_functions.h>
#include <__execution/completion_attrs.h>
#include <__execution/completion_signatures.h>
#include <__execution/fwd_env.h>
#include <__execution/get_completion_signatures.h>
#include <__execution/get_env.h>
#include <__execution/just.h>
#include <__execution/let.h>
#include <__execution/movable_value.h>
#include <__execution/sender.h>
#include <__execution/sender_adaptor_closure.h>
#include <__functional/bind_back.h>
#include <__type_traits/decay.h>
#include <__type_traits/is_nothrow_constructible.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/forward.h>
#include <__utility/forward_like.h>
#include <__utility/move.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_PUSH_MACROS
#include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

namespace execution {

// [exec.stopped.err]: stopped_as_error(sndr, err) is make-sender(stopped_as_error, err, sndr), whose transformation is
// let_stopped(child, [err = forward_like<Sndr>(err)]() mutable noexcept(...) { return just_error(std::move(err)); }):
// the result of the call is an aggregate with public `tag`/`data`/`child` members that is lowered by transform_sender
// when it is connected to a receiver whose domain does not customize stopped_as_error.
struct stopped_as_error_t;

template <class _Err, class _Sndr>
class __stopped_as_error_sndr;

struct stopped_as_error_t {
  template <sender _Sndr, __movable_value _Err>
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Sndr&& __sndr, _Err&& __err) const;

  template <class _Err>
    requires constructible_from<decay_t<_Err>, _Err>
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Err&& __err) const
      noexcept(is_nothrow_constructible_v<decay_t<_Err>, _Err>) {
    return execution::__pipeable(std::__bind_back(*this, std::forward<_Err>(__err)));
  }

  // [exec.stopped.err]p4: stopped_as_error.transform_sender(set_value, sndr, env), for a sender of this tag.
  template <class _Sndr, class _Env>
    requires __sender_for<_Sndr, stopped_as_error_t>
  _LIBCPP_HIDE_FROM_ABI static constexpr auto transform_sender(set_value_t, _Sndr&& __sndr, const _Env&) {
    auto&& [__tag, __err, __child] = __sndr;
    using _Error = decltype(auto(__err));
    return execution::let_stopped(
        std::forward_like<_Sndr>(__child),
        [__e = std::forward_like<_Sndr>(__err)]() mutable noexcept(is_nothrow_move_constructible_v<_Error>) {
          return execution::just_error(std::move(__e));
        });
  }
};

// The completions of the child with tag set_stopped become error completions (just_error(err) after let_stopped), in the
// same place; the other completions are forwarded.
struct __stopped_as_error_contrib {
  template <class _ChildSigs, class _Out>
  static consteval unsigned __mask() {
    return __intercept_contributors<set_stopped_t, set_error_t, false, _ChildSigs>::template __mask<_Out>();
  }
};

template <class _Err, class _Sndr>
class __stopped_as_error_sndr {
public:
  using sender_concept = sender_tag;

  _LIBCPP_NO_UNIQUE_ADDRESS stopped_as_error_t tag;
  _Err data;
  _Sndr child;

  _LIBCPP_HIDE_FROM_ABI constexpr auto get_env() const noexcept {
    using __child_attrs_t = remove_cvref_t<decltype(execution::get_env(child))>;
    return __completion_attrs<__stopped_as_error_contrib, _Sndr, __child_attrs_t>(execution::get_env(child));
  }

  // Only reached for a sender that was not transformed, which cannot happen: the transformation has no constraints
  // but the shape of the sender.
  template <class _Self, class... _Env>
  _LIBCPP_HIDE_FROM_ABI static consteval auto get_completion_signatures() {
    if constexpr (sizeof...(_Env) == 0) {
      return execution::__lowered_signatures_without_env<_Self>();
    } else {
      throw __unspecified_exception();
      return completion_signatures<>();
    }
  }
};

template <sender _Sndr, __movable_value _Err>
_LIBCPP_HIDE_FROM_ABI constexpr auto stopped_as_error_t::operator()(_Sndr&& __sndr, _Err&& __err) const {
  return __stopped_as_error_sndr<decay_t<_Err>, remove_cvref_t<_Sndr>>{
      {}, decay_t<_Err>(std::forward<_Err>(__err)), std::forward<_Sndr>(__sndr)};
}

inline constexpr stopped_as_error_t stopped_as_error{};

} // namespace execution

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP___EXECUTION_STOPPED_AS_ERROR_H
