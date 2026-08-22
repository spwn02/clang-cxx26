//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___EXECUTION_INTO_VARIANT_H
#define _LIBCPP___EXECUTION_INTO_VARIANT_H

#include <__config>
#include <__execution/completion_signatures.h>
#include <__execution/connect.h>
#include <__execution/fwd_env.h>
#include <__execution/get_completion_signatures.h>
#include <__execution/get_env.h>
#include <__execution/receiver.h>
#include <__execution/sender.h>
#include <__execution/sender_adaptor_closure.h>
#include <__type_traits/is_nothrow_constructible.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/forward.h>
#include <__utility/move.h>
#include <exception>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

namespace execution {

// [exec.into.variant]. into_variant(sndr) adapts a sender with (possibly multiple) value
// completion signatures into a sender with exactly one value completion signature,
// `set_value_t(V)`, where `V` is `value_types_of_t<child, Env>` -- a variant of one
// decayed-tuple per distinct value-completion shape (this is precisely
// <__execution/get_completion_signatures.h>'s value_types_of_t called with its default
// Tuple/Variant arguments, __decayed_tuple/__variant_or_empty -- no new gathering machinery
// needed). Every set_error_t/set_stopped_t completion passes through unchanged. Like
// <__execution/stopped_as_optional.h>'s V, this V genuinely depends on Env (not known until
// connect()/get_completion_signatures() see a real receiver/queried environment), so this
// needs its own hand-rolled sender type rather than a pure call-time composition -- same
// precedent, not routed through the draft's basic-sender/impls-for/make-sender machinery (see
// the M3 entry in docs/CXX26_GAPS.md for why that engine isn't buildable on this fork yet).
//
// [exec.into.variant]p4's check-types (decay-copyable-result-datums) is not implemented --
// same P3068 constexpr-exceptions gap as every other adaptor in this sub-plan; a child sender
// whose value datums aren't all decay-copyable simply makes get_completion_signatures's
// __decayed_tuple instantiation itself ill-formed rather than reporting a dedicated
// diagnostic (the same "hard error instead of a clean Mandates diagnostic" shape already
// documented for every other adaptor here, not a new gap).
//
// into_variant_t is defined in full first (its operator() only *declared*, trailing-return-
// type naming the not-yet-complete __into_variant_sndr, which is fine -- a trailing return
// type doesn't require completeness until the function is actually called/defined), then
// __into_variant_sndr is defined in full (needing into_variant_t as a complete, non-dependent
// `tag` member), then into_variant_t::operator()'s body is defined out-of-line, after
// __into_variant_sndr is complete -- matching <__execution/stopped_as_optional.h>'s/
// <__execution/write_env.h>'s own ordering, for the identical reason.
template <class _Sndr>
class __into_variant_sndr;

struct into_variant_t : sender_adaptor_closure<into_variant_t> {
  template <sender _Sndr>
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Sndr&& __sndr) const
      -> __into_variant_sndr<remove_cvref_t<_Sndr>>;
};

inline constexpr into_variant_t into_variant{};

template <class _VariantT, class _Rcvr>
class __into_variant_rcvr {
public:
  using receiver_concept = receiver_tag;

  _LIBCPP_HIDE_FROM_ABI constexpr explicit __into_variant_rcvr(_Rcvr&& __rcvr) : __rcvr_(std::move(__rcvr)) {}

  // [exec.into.variant]p6: TRY-SET-VALUE(rcvr, variant_type(decayed-tuple<Args...>{args...})).
  // Constructing the intermediate decayed-tuple and converting it into the (deduped) variant
  // can each throw -- unlike <__execution/stopped_as_optional.h>'s single-datum V, this is a
  // genuine two-step construction, so the nothrow check covers both steps rather than reusing
  // a one-argument is_nothrow_constructible_v the way that file does.
  template <class... _Args>
  _LIBCPP_HIDE_FROM_ABI constexpr void set_value(_Args&&... __args) && noexcept {
    if constexpr (is_nothrow_constructible_v<__decayed_tuple<_Args...>, _Args...> &&
                  is_nothrow_constructible_v<_VariantT, __decayed_tuple<_Args...>>) {
      execution::set_value(std::move(__rcvr_), _VariantT(__decayed_tuple<_Args...>(std::forward<_Args>(__args)...)));
    } else {
      try {
        execution::set_value(std::move(__rcvr_), _VariantT(__decayed_tuple<_Args...>(std::forward<_Args>(__args)...)));
      } catch (...) {
        execution::set_error(std::move(__rcvr_), std::current_exception());
      }
    }
  }

  template <class _Err>
  _LIBCPP_HIDE_FROM_ABI constexpr void set_error(_Err&& __err) && noexcept {
    execution::set_error(std::move(__rcvr_), std::forward<_Err>(__err));
  }

  _LIBCPP_HIDE_FROM_ABI constexpr void set_stopped() && noexcept { execution::set_stopped(std::move(__rcvr_)); }

  _LIBCPP_HIDE_FROM_ABI constexpr auto get_env() const noexcept {
    return execution::__fwd_env_fn(execution::get_env(__rcvr_));
  }

private:
  _Rcvr __rcvr_;
};

// [exec.into.variant]p4's completion-signature transform: every set_value_t(Args...)
// signature is dropped (each is subsumed by the single combined set_value_t(_VariantT)
// signature prepended below); every set_error_t/set_stopped_t signature passes through
// unchanged. Unlike <__execution/then.h>'s and <__execution/let.h>'s transforms, there is
// nothing to dedup here: the set_value_t signatures are removed outright rather than
// remapped, and only one is ever added back.
template <class _VariantT, class _Completions>
struct __into_variant_signatures;

template <class _VariantT, class... _Fns>
struct __into_variant_signatures<_VariantT, completion_signatures<_Fns...>> {
  template <class _Sig>
  struct __one {
    using type = type_list<_Sig>;
  };
  template <class... _Args>
  struct __one<set_value_t(_Args...)> {
    using type = type_list<>;
  };

  using __gathered =
      typename __concat_type_lists<type_list<set_value_t(_VariantT)>, typename __one<_Fns>::type...>::type;

  template <class _List>
  struct __to_completion_signatures;
  template <class... _Sigs>
  struct __to_completion_signatures<type_list<_Sigs...>> {
    using type = completion_signatures<_Sigs...>;
  };

  using type = typename __to_completion_signatures<__gathered>::type;
};

// An aggregate with public `tag`/`child` members, matching the (tag, data, ...children) shape
// tag_of_t (<__execution/sender.h>) decomposes via structured bindings -- into_variant has no
// data, so this decomposes as (tag, child) per [exec.snd.general]p3's "no data" convention
// used identically by <__execution/stopped_as_optional.h>/<__execution/stopped_as_error.h>.
template <class _Sndr>
class __into_variant_sndr {
public:
  using sender_concept = sender_tag;

  _LIBCPP_NO_UNIQUE_ADDRESS into_variant_t tag;
  _Sndr child;

  template <class _Rcvr>
  _LIBCPP_HIDE_FROM_ABI constexpr auto connect(_Rcvr&& __rcvr) && {
    using __env_t     = __fwd_env<env_of_t<remove_cvref_t<_Rcvr>>>;
    using __variant_t = value_types_of_t<_Sndr, __env_t>;
    return execution::connect(
        std::move(child), __into_variant_rcvr<__variant_t, remove_cvref_t<_Rcvr>>(std::forward<_Rcvr>(__rcvr)));
  }

  // [exec.adapt.general]p3.2: a parent sender with a single child sndr has an associated
  // attribute object equal to FWD-ENV(get_env(sndr)).
  _LIBCPP_HIDE_FROM_ABI constexpr auto get_env() const noexcept {
    return execution::__fwd_env_fn(execution::get_env(child));
  }

  template <class _Self, class _Env>
    requires sender_in<_Sndr, __fwd_env<remove_cvref_t<_Env>>>
  _LIBCPP_HIDE_FROM_ABI static consteval auto get_completion_signatures() {
    using __env_t       = __fwd_env<remove_cvref_t<_Env>>;
    using __variant_t   = value_types_of_t<_Sndr, __env_t>;
    using __child_sigs  = completion_signatures_of_t<_Sndr, __env_t>;
    return typename __into_variant_signatures<__variant_t, __child_sigs>::type{};
  }
};

template <sender _Sndr>
_LIBCPP_HIDE_FROM_ABI constexpr auto into_variant_t::operator()(_Sndr&& __sndr) const
    -> __into_variant_sndr<remove_cvref_t<_Sndr>> {
  return __into_variant_sndr<remove_cvref_t<_Sndr>>{{}, std::forward<_Sndr>(__sndr)};
}

} // namespace execution

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___EXECUTION_INTO_VARIANT_H
