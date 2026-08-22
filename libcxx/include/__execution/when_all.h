//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___EXECUTION_WHEN_ALL_H
#define _LIBCPP___EXECUTION_WHEN_ALL_H

#include <__config>
#include <__execution/completion_signatures.h>
#include <__execution/connect.h>
#include <__execution/forwarding_query.h>
#include <__execution/fwd_env.h>
#include <__execution/get_completion_signatures.h>
#include <__execution/get_env.h>
#include <__execution/get_stop_token.h>
#include <__execution/operation_state.h>
#include <__execution/receiver.h>
#include <__execution/sender.h>
#include <__stop_token/inplace_stop_callback.h>
#include <__stop_token/inplace_stop_source.h>
#include <__stop_token/inplace_stop_token.h>
#include <__stop_token/stoppable_token.h>
#include <__type_traits/conditional.h>
#include <__type_traits/decay.h>
#include <__type_traits/invoke.h>
#include <__type_traits/is_nothrow_constructible.h>
#include <__type_traits/is_same.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/forward.h>
#include <__utility/integer_sequence.h>
#include <__utility/move.h>
#include <atomic>
#include <exception>
#include <optional>
#include <tuple>
#include <variant>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_THREADS

namespace execution {

// [exec.when.all]. when_all(sndrs...) adapts multiple input senders into a sender that
// completes when all input senders have completed: on success, it concatenates every input
// sender's value-completion datums into one value completion; on the first error or stopped
// completion from any child, it requests stop on every other child and, once all have
// finished, completes with that error (or stopped, if no error occurred). Genuinely the first
// *multi*-child adaptor in this sub-plan (M1 through the rest of M5 have all had exactly one
// child) -- hand-rolled per this sub-plan's established precedent (no impls-for/basic-sender
// engine on this fork; see the M3 entry in docs/CXX26_GAPS.md), but structurally new: shared
// mutable state across N concurrently-racing child operations, not a single linear pipeline.
//
// `when_all_with_variant` (p18/p19 of the clause -- a pure call-time composition,
// `when_all(into_variant(sndrs)...)`) is deliberately deferred to a follow-up commit; this
// file implements `when_all` alone.
//
// check-types ([exec.when.all]p8/p9, the Mandates-throwing consteval helper that diagnoses a
// child with 2+ set_value completions) is not implemented -- same P3068 constexpr-exceptions
// gap as every other adaptor in this sub-plan. Per p13's own literal wording, a child with
// zero *or* two-or-more set_value shapes both make `value_types_of_t<..., optional>` (the
// `_Variant` argument spelled `optional`, which only accepts exactly one type argument)
// ill-formed to name -- so both cases collapse into the same "otherwise tuple<>" fallback
// (values_tuple becomes tuple<>, and this when_all's own value completion becomes the
// datum-less set_value_t()) rather than a hard Mandates diagnostic for the 2+ case
// specifically. Implemented by classifying each child's own completion_signatures via
// ordinary partial specialization on the always-well-formed shape gathered by
// value_types_of_t<..., __decayed_tuple, type_list> (0, 1, or N elements, never ill-formed
// regardless of arity), rather than naming `optional<Ts...>` directly for the wrong arity --
// the exact "gather into something always well-formed, then pattern-match the shape" lesson
// <__execution/get_completion_signatures.h>'s __single_sender_value_type already established
// (naming the ill-formed alias directly hard-errors deep inside __gather_signatures_impl's
// implicit instantiation, not SFINAE-safely, for the same reason recorded there).

enum class __when_all_disposition { __started, __error, __stopped };

// [exec.when.all]p12: "none-such", an unspecified empty class type used as the errors_variant
// placeholder for "no error yet" and, when no child's datums can throw on decay-copy, for
// copy-fail too.
struct __when_all_none_such {};

// ---------------------------------------------------------------------------------------------
// when-all-env ([exec.when.all]p5-7): a queryable wrapping the outer receiver's own (forwarded)
// environment, except that get_stop_token is intercepted to answer with this operation's own
// inplace_stop_source's token (not whatever the outer environment would otherwise answer) --
// this is what lets requesting stop on one child propagate to every other still-running child.
// Modeled directly on <__execution/fwd_env.h>'s FWD-ENV query-forwarding shape, with one
// query overridden ahead of the generic forward.
template <class _Env>
class __when_all_env {
public:
  _LIBCPP_HIDE_FROM_ABI constexpr __when_all_env(inplace_stop_source& __stop_src, _Env __env) noexcept(
      is_nothrow_move_constructible_v<_Env>)
      : __stop_src_(&__stop_src), __env_(std::move(__env)) {}

  _LIBCPP_HIDE_FROM_ABI constexpr inplace_stop_token query(get_stop_token_t) const noexcept {
    return __stop_src_->get_token();
  }

  // Parenthesized per <__execution/fwd_env.h>'s own note: a bare CPO call as the first
  // operand of a `requires`-clause immediately followed by `&&` mis-parses on this fork's
  // Clang unless parenthesized.
  template <class _Tag, class... _Args>
    requires(!same_as<_Tag, get_stop_token_t>) && (std::forwarding_query(_Tag())) &&
            requires(const _Env& __env, _Tag __tag, _Args&&... __args) {
              __env.query(__tag, std::forward<_Args>(__args)...);
            }
  _LIBCPP_HIDE_FROM_ABI constexpr decltype(auto) query(_Tag __tag, _Args&&... __args) const
      noexcept(noexcept(__env_.query(__tag, std::forward<_Args>(__args)...))) {
    return __env_.query(__tag, std::forward<_Args>(__args)...);
  }

private:
  inplace_stop_source* __stop_src_;
  _Env __env_;
};

template <class _Env>
_LIBCPP_HIDE_FROM_ABI constexpr auto __make_when_all_env(inplace_stop_source& __stop_src, _Env&& __env) noexcept(
    is_nothrow_constructible_v<__when_all_env<remove_cvref_t<_Env>>, inplace_stop_source&, _Env>) {
  return __when_all_env<remove_cvref_t<_Env>>(__stop_src, std::forward<_Env>(__env));
}

// The callback registered against the *outer* receiver's own stop token ([exec.when.all]p16):
// when that fires, propagate into this when_all's own inplace_stop_source, which is what
// __when_all_env answers get_stop_token with for every child.
class __when_all_on_stop_request {
public:
  _LIBCPP_HIDE_FROM_ABI explicit __when_all_on_stop_request(inplace_stop_source& __stop_src) noexcept
      : __stop_src_(&__stop_src) {}
  _LIBCPP_HIDE_FROM_ABI void operator()() noexcept { __stop_src_->request_stop(); }

private:
  inplace_stop_source* __stop_src_;
};

// ---------------------------------------------------------------------------------------------
// Per-child value-shape classification ([exec.when.all]p13's "if that type is well-formed;
// otherwise tuple<>", made SFINAE-safe -- see the file-level comment above).
struct __when_all_no_single_value {}; // 0 set_value_t signatures.
struct __when_all_multiple_value {};  // 2+ set_value_t signatures.

template <class... _Args>
struct __when_all_single_value {
  using __tuple                   = __decayed_tuple<_Args...>;
  static constexpr bool __nothrow = is_nothrow_constructible_v<__tuple, _Args...>;
};

template <class _Acc, class _Fn>
struct __when_all_shape_step {
  using type = _Acc; // non-set_value_t signature: accumulator unchanged.
};
template <class _Acc, class... _Args>
struct __when_all_shape_step<_Acc, set_value_t(_Args...)> {
  using type = __conditional_t<is_same_v<_Acc, __when_all_no_single_value>,
                                __when_all_single_value<_Args...>,
                                __when_all_multiple_value>;
};

template <class _Acc, class... _Fns>
struct __when_all_fold_shape {
  using type = _Acc;
};
template <class _Acc, class _Fn, class... _Rest>
struct __when_all_fold_shape<_Acc, _Fn, _Rest...>
    : __when_all_fold_shape<typename __when_all_shape_step<_Acc, _Fn>::type, _Rest...> {};

template <class _Completions>
struct __when_all_shape_from_sigs;
template <class... _Fns>
struct __when_all_shape_from_sigs<completion_signatures<_Fns...>> {
  using type = typename __when_all_fold_shape<__when_all_no_single_value, _Fns...>::type;
};

template <class _Sndr, class _Env>
using __when_all_child_shape_t = typename __when_all_shape_from_sigs<completion_signatures_of_t<_Sndr, _Env>>::type;

template <class _T>
inline constexpr bool __when_all_is_single_value_v = false;
template <class... _Args>
inline constexpr bool __when_all_is_single_value_v<__when_all_single_value<_Args...>> = true;

template <class _Env, class... _Sndrs>
inline constexpr bool __when_all_all_single_value =
    (__when_all_is_single_value_v<__when_all_child_shape_t<_Sndrs, _Env>> && ...);

// values_tuple: tuple<> unless every child has exactly one value shape, in which case
// tuple<optional<Tuple_1>, ..., optional<Tuple_N>> (one slot per child, in order).
template <bool _AllSingle, class _Env, class... _Sndrs>
struct __when_all_values_tuple_impl {
  using type = tuple<>;
};
template <class _Env, class... _Sndrs>
struct __when_all_values_tuple_impl<true, _Env, _Sndrs...> {
  using type = tuple<optional<typename __when_all_child_shape_t<_Sndrs, _Env>::__tuple>...>;
};
template <class _Env, class... _Sndrs>
using __when_all_values_tuple =
    typename __when_all_values_tuple_impl<__when_all_all_single_value<_Env, _Sndrs...>, _Env, _Sndrs...>::type;

// The value completion signature: set_value_t() if values_tuple is tuple<>, otherwise
// set_value_t(AllArgs...) -- every child's own Args, concatenated in order.
template <class _Tuple>
struct __when_all_tuple_to_list;
template <class... _Args>
struct __when_all_tuple_to_list<tuple<_Args...>> {
  using type = type_list<_Args...>;
};

template <bool _AllSingle, class _Env, class... _Sndrs>
struct __when_all_value_sig_impl {
  using type = set_value_t();
};
template <class _Env, class... _Sndrs>
struct __when_all_value_sig_impl<true, _Env, _Sndrs...> {
  using __combined = typename __concat_type_lists<
      typename __when_all_tuple_to_list<typename __when_all_child_shape_t<_Sndrs, _Env>::__tuple>::type...>::type;

  template <class _List>
  struct __to_sig;
  template <class... _Args>
  struct __to_sig<type_list<_Args...>> {
    using type = set_value_t(_Args...);
  };

  using type = typename __to_sig<__combined>::type;
};
template <class _Env, class... _Sndrs>
using __when_all_value_sig =
    typename __when_all_value_sig_impl<__when_all_all_single_value<_Env, _Sndrs...>, _Env, _Sndrs...>::type;

// ---------------------------------------------------------------------------------------------
// [exec.when.all]p12: copy-fail is exception_ptr if decay-copying any of the child senders'
// result datums -- value *or* error, this is why TRY-EMPLACE-ERROR's own fallback can assume
// exception_ptr is always a valid errors_variant alternative whenever it might actually throw
// -- can potentially throw; otherwise none-such. Independent of the all-single-value dispatch
// above: this scans every set_value_t/set_error_t signature of every child, not just the ones
// that end up contributing to values_tuple.
template <class _Fn>
inline constexpr bool __when_all_sig_nothrow_copy_v = true; // set_stopped_t() and anything else: no datums.
template <class... _Args>
inline constexpr bool __when_all_sig_nothrow_copy_v<set_value_t(_Args...)> =
    is_nothrow_constructible_v<__decayed_tuple<_Args...>, _Args...>;
template <class _Err>
inline constexpr bool __when_all_sig_nothrow_copy_v<set_error_t(_Err)> = is_nothrow_constructible_v<decay_t<_Err>, _Err>;

template <class _Completions>
struct __when_all_all_nothrow_copy;
template <class... _Fns>
struct __when_all_all_nothrow_copy<completion_signatures<_Fns...>> {
  static constexpr bool value = (__when_all_sig_nothrow_copy_v<_Fns> && ...);
};

template <class _Env, class... _Sndrs>
inline constexpr bool __when_all_all_children_nothrow_copy =
    (__when_all_all_nothrow_copy<completion_signatures_of_t<_Sndrs, _Env>>::value && ...);

template <class _Env, class... _Sndrs>
using __when_all_copy_fail_t =
    __conditional_t<__when_all_all_children_nothrow_copy<_Env, _Sndrs...>, __when_all_none_such, exception_ptr>;

// errors_variant: variant<none-such, copy-fail, Es...>, deduped, where Es is every child's own
// decayed error types (via the existing error_types_of_t alias, gathered as a type_list so it
// concatenates with <__execution/completion_signatures.h>'s __concat_type_lists directly).
template <class _Env, class... _Sndrs>
using __when_all_errors_concat_t = typename __concat_type_lists<
    type_list<__when_all_none_such, __when_all_copy_fail_t<_Env, _Sndrs...>>,
    error_types_of_t<_Sndrs, _Env, type_list>...>::type;

template <class _List>
struct __when_all_dedup_errors;
template <class... _Ts>
struct __when_all_dedup_errors<type_list<_Ts...>> {
  template <class>
  struct __to_variant;
  template <class... _Us>
  struct __to_variant<type_list<_Us...>> {
    using type = variant<_Us...>;
  };
  using type = typename __to_variant<__dedup_type_list_t<_Ts...>>::type;
};

template <class _Env, class... _Sndrs>
using __when_all_errors_variant = typename __when_all_dedup_errors<__when_all_errors_concat_t<_Env, _Sndrs...>>::type;

// [exec.when.all]p15.3: sends-stopped is true iff some child's own completion signatures
// include set_stopped_t().
template <class _Env, class... _Sndrs>
inline constexpr bool __when_all_sends_stopped = (sends_stopped<_Sndrs, _Env> || ...);

// Strips the leading none-such alternative every __when_all_errors_variant always carries
// (dedup preserves first-seen order, and none-such is always fed in first) and maps every
// remaining alternative E to a set_error_t(E) completion signature.
template <class _ErrorsVariant>
struct __when_all_error_sigs_from_variant;
template <class... _Es>
struct __when_all_error_sigs_from_variant<variant<__when_all_none_such, _Es...>> {
  using type = completion_signatures<set_error_t(_Es)...>;
};

template <class _Sigs>
struct __when_all_sigs_to_list;
template <class... _Fns>
struct __when_all_sigs_to_list<completion_signatures<_Fns...>> {
  using type = type_list<_Fns...>;
};

template <class _List>
struct __when_all_list_to_sigs;
template <class... _Fns>
struct __when_all_list_to_sigs<type_list<_Fns...>> {
  using type = completion_signatures<_Fns...>;
};

template <bool _SendsStopped, class _ValueSig, class _ErrorSigs>
struct __when_all_final_sigs_impl {
  using type = typename __when_all_list_to_sigs<
      typename __concat_type_lists<type_list<_ValueSig>, typename __when_all_sigs_to_list<_ErrorSigs>::type>::type>::type;
};
template <class _ValueSig, class _ErrorSigs>
struct __when_all_final_sigs_impl<true, _ValueSig, _ErrorSigs> {
  using type = typename __when_all_list_to_sigs<typename __concat_type_lists<
      type_list<_ValueSig>, typename __when_all_sigs_to_list<_ErrorSigs>::type, type_list<set_stopped_t()>>::type>::type;
};

template <class _Env, class... _Sndrs>
using __when_all_completion_signatures = typename __when_all_final_sigs_impl<
    __when_all_sends_stopped<_Env, _Sndrs...>,
    __when_all_value_sig<_Env, _Sndrs...>,
    typename __when_all_error_sigs_from_variant<__when_all_errors_variant<_Env, _Sndrs...>>::type>::type;

// ---------------------------------------------------------------------------------------------
// Shared operation state ([exec.when.all]p11). `_Env` here is __fwd_env<env_of_t<Rcvr>> --
// the same environment get_completion_signatures computes against -- so values_tuple/
// errors_variant/sends_stopped here are guaranteed to match what was statically advertised.
// stop_token_of_t<_Env> equals stop_token_of_t<env_of_t<Rcvr>> since FWD-ENV forwards
// forwarding-queries and get_stop_token_t derives from forwarding_query_t, so this needs no
// separate template parameter for the outer receiver's own stop-token type.
template <class _Env, class... _Sndrs>
struct __when_all_state {
  using __values_tuple_t   = __when_all_values_tuple<_Env, _Sndrs...>;
  using __errors_variant_t = __when_all_errors_variant<_Env, _Sndrs...>;
  using __stop_callback_t  = stop_callback_for_t<stop_token_of_t<_Env>, __when_all_on_stop_request>;

  static constexpr bool __sends_stopped = __when_all_sends_stopped<_Env, _Sndrs...>;

  atomic<size_t> __count_{sizeof...(_Sndrs)};
  inplace_stop_source __stop_src_{};
  atomic<__when_all_disposition> __disp_{__when_all_disposition::__started};
  __errors_variant_t __errors_{};
  __values_tuple_t __values_{};
  optional<__stop_callback_t> __on_stop_{nullopt};

  template <class _Rcvr>
  _LIBCPP_HIDE_FROM_ABI void __arrive(_Rcvr& __rcvr) noexcept {
    if (--__count_ == 0) {
      __complete(__rcvr);
    }
  }

  // [exec.when.all]p15.
  template <class _Rcvr>
  _LIBCPP_HIDE_FROM_ABI void __complete(_Rcvr& __rcvr) noexcept {
    __when_all_disposition __d = __disp_.load();
    if (__d == __when_all_disposition::__started) {
      __on_stop_.reset();
      auto __tie = []<class... _T>(tuple<_T...>& __t) noexcept { return tuple<_T&...>(__t); };
      std::apply(
          [&](auto&... __opts) noexcept {
            std::apply(
                [&](auto&... __args) noexcept { execution::set_value(std::move(__rcvr), std::move(__args)...); },
                std::tuple_cat(__tie(*__opts)...));
          },
          __values_);
    } else if (__d == __when_all_disposition::__error) {
      __on_stop_.reset();
      std::visit(
          [&]<class _Error>(_Error& __error) noexcept {
            if constexpr (!is_same_v<_Error, __when_all_none_such>) {
              execution::set_error(std::move(__rcvr), std::move(__error));
            }
          },
          __errors_);
    } else {
      if constexpr (__sends_stopped) {
        __on_stop_.reset();
        execution::set_stopped(std::move(__rcvr));
      }
    }
  }
};

// ---------------------------------------------------------------------------------------------
// Per-child receiver ([exec.when.all]p17). Stores direct pointers to the (already-complete,
// ordinary) shared State and outer Rcvr -- not a pointer back to the enclosing operation-state
// class template, which is what <__execution/let.h>'s own "real engineering hazard" note
// (docs/CXX26_GAPS.md, M5) warns is unsafe while that enclosing template is still incomplete
// during a child's own connect()-time constraint-checking. State and Rcvr are both ordinary,
// already-complete types at the point children are connected, so this sidesteps the hazard
// entirely rather than needing let.h's two-member workaround.
template <size_t _Index, class _State, class _Rcvr>
class __when_all_rcvr {
public:
  using receiver_concept = receiver_tag;

  _LIBCPP_HIDE_FROM_ABI constexpr __when_all_rcvr(_State* __state, _Rcvr* __rcvr) noexcept
      : __state_(__state), __rcvr_(__rcvr) {}

  template <class... _Args>
  _LIBCPP_HIDE_FROM_ABI constexpr void set_value(_Args&&... __args) && noexcept {
    __complete(set_value_t{}, std::forward<_Args>(__args)...);
  }
  template <class _Err>
  _LIBCPP_HIDE_FROM_ABI constexpr void set_error(_Err&& __err) && noexcept {
    __complete(set_error_t{}, std::forward<_Err>(__err));
  }
  _LIBCPP_HIDE_FROM_ABI constexpr void set_stopped() && noexcept { __complete(set_stopped_t{}); }

  _LIBCPP_HIDE_FROM_ABI constexpr auto get_env() const noexcept {
    return execution::__make_when_all_env(__state_->__stop_src_, execution::__fwd_env_fn(execution::get_env(*__rcvr_)));
  }

private:
  // [exec.when.all]p17's TRY-EMPLACE-ERROR(errors, e): decay-copy e into errors, falling back
  // to exception_ptr on a throwing decay-copy -- always a valid alternative when it might
  // actually throw, since copy-fail (see above) scans every child's error datums too.
  template <class _Err>
  _LIBCPP_HIDE_FROM_ABI constexpr void __try_emplace_error(_Err&& __err) noexcept {
    if constexpr (is_nothrow_constructible_v<decay_t<_Err>, _Err>) {
      __state_->__errors_.template emplace<decay_t<_Err>>(std::forward<_Err>(__err));
    } else {
      try {
        __state_->__errors_.template emplace<decay_t<_Err>>(std::forward<_Err>(__err));
      } catch (...) {
        __state_->__errors_.template emplace<exception_ptr>(std::current_exception());
      }
    }
  }

  // [exec.when.all]p17's TRY-EMPLACE-VALUE(complete, opt, args...): on a throwing decay-copy,
  // recurse into __complete with set_error_t(current_exception()) instead -- which itself
  // calls __arrive at its own end -- and report back so the caller skips its own __arrive
  // (exactly one __arrive per child, matching the standard's own recursive-return-early shape;
  // see the file-level TRY-EMPLACE-VALUE recursion note).
  template <class... _Args>
  _LIBCPP_HIDE_FROM_ABI constexpr bool __try_emplace_value(optional<__decayed_tuple<remove_cvref_t<_Args>...>>& __opt,
                                                            _Args&&... __args) noexcept {
    if constexpr (is_nothrow_constructible_v<__decayed_tuple<remove_cvref_t<_Args>...>, _Args...>) {
      __opt.emplace(std::forward<_Args>(__args)...);
      return false;
    } else {
      try {
        __opt.emplace(std::forward<_Args>(__args)...);
      } catch (...) {
        __complete(set_error_t{}, std::current_exception());
        return true;
      }
      return false;
    }
  }

  template <class _Tag, class... _Args>
  _LIBCPP_HIDE_FROM_ABI constexpr void __complete(_Tag, _Args&&... __args) noexcept {
    if constexpr (same_as<_Tag, set_error_t>) {
      if (__state_->__disp_.exchange(__when_all_disposition::__error) != __when_all_disposition::__error) {
        __state_->__stop_src_.request_stop();
        __try_emplace_error(std::forward<_Args>(__args)...);
      }
    } else if constexpr (same_as<_Tag, set_stopped_t>) {
      auto __expected = __when_all_disposition::__started;
      if (__state_->__disp_.compare_exchange_strong(__expected, __when_all_disposition::__stopped)) {
        __state_->__stop_src_.request_stop();
      }
    } else if constexpr (!is_same_v<typename _State::__values_tuple_t, tuple<>>) {
      if (__state_->__disp_.load() == __when_all_disposition::__started) {
        auto& __opt = std::get<_Index>(__state_->__values_);
        if (__try_emplace_value(__opt, std::forward<_Args>(__args)...)) {
          return; // the recursive set_error path above already called __arrive.
        }
      }
    }
    __state_->__arrive(*__rcvr_);
  }

  _State* __state_;
  _Rcvr* __rcvr_;
};

// Operation states are neither movable nor copyable (matching every other opstate in this
// sub-plan -- e.g. __just_opstate, __let_opstate). Constructing tuple<OpStates...> directly
// from N execution::connect(...) calls would materialize each call's prvalue result through
// std::tuple's own forwarding-reference constructor parameter, defeating guaranteed copy
// elision -- the exact same hazard <__execution/let.h>'s __emplace_from documents at length.
// Same fix: a thin wrapper with `operator T() &&` that calls the factory, so the *wrapper*
// (cheap, trivially movable) is what gets forwarded through tuple's constructor hops, and the
// actual non-movable OpState is only ever produced at the final direct-initialization step
// (`Tp elem(wrapper)` resolving to `wrapper.operator Tp()`), which mandatory elision does
// cover (a prvalue of the same type as the object being initialized).
template <class _Fn>
struct __when_all_emplace_from {
  _Fn __fn_;
  using __result_t = invoke_result_t<_Fn&>;
  _LIBCPP_HIDE_FROM_ABI constexpr operator __result_t() && { return std::move(__fn_)(); }
};
template <class _Fn>
__when_all_emplace_from(_Fn) -> __when_all_emplace_from<_Fn>;

// ---------------------------------------------------------------------------------------------
// Operation state: connects every child against its own __when_all_rcvr<Index, State, Rcvr>,
// keeping each child's resulting operation state in a tuple alongside the shared State and the
// (single, shared) outer Rcvr. Declaration order matters here: __state_ and __rcvr_ must both
// be fully constructed before __ops_ (whose per-child connect() calls take `&__state_`/
// `&__rcvr_`), and member initialization follows declaration order, not mem-initializer-list
// order.
template <class _Rcvr, class _Iseq, class... _Sndrs>
class __when_all_opstate;

template <class _Rcvr, size_t... _Is, class... _Sndrs>
class __when_all_opstate<_Rcvr, index_sequence<_Is...>, _Sndrs...> {
public:
  using operation_state_concept = operation_state_tag;

  template <class _URcvr>
  _LIBCPP_HIDE_FROM_ABI constexpr __when_all_opstate(tuple<_Sndrs...>&& __children, _URcvr&& __rcvr)
      : __rcvr_(std::forward<_URcvr>(__rcvr)),
        __ops_(__when_all_emplace_from{[this, &__children] {
          return execution::connect(std::get<_Is>(std::move(__children)),
                                     __when_all_rcvr<_Is, __state_t, _Rcvr>(&__state_, &__rcvr_));
        }}...) {}

  _LIBCPP_HIDE_FROM_ABI constexpr void start() & noexcept {
    __state_.__on_stop_.emplace(std::get_stop_token(execution::get_env(__rcvr_)),
                                 __when_all_on_stop_request(__state_.__stop_src_));
    (execution::start(std::get<_Is>(__ops_)), ...);
  }

private:
  using __env_t   = __fwd_env<env_of_t<_Rcvr>>;
  using __state_t = __when_all_state<__env_t, _Sndrs...>;

  __state_t __state_{};
  _Rcvr __rcvr_;
  tuple<connect_result_t<_Sndrs, __when_all_rcvr<_Is, __state_t, _Rcvr>>...> __ops_;
};

// ---------------------------------------------------------------------------------------------
// An aggregate with public `tag`/`children` members. Deliberately does *not* satisfy
// tag_of_t's (tag, data, ...children) structured-binding decomposition -- confirmed by reading
// <__execution/domain.h>'s default_domain::transform_sender in full: its tag-dispatch branch
// (the only caller of tag_of_t anywhere in this tree) unconditionally takes the "otherwise"
// path per the M2 deviation recorded there, so tag_of_t is never actually invoked on any
// sender's connect()/get_completion_signatures() path in this fork. Same precedent as
// <__execution/starts_on.h>/<__execution/stopped_as_error.h>/<__execution/schedule_from.h>,
// which document the identical omission for the same reason.
//
// when_all_t is defined in full first (its operator() only *declared*, trailing-return-type
// naming the not-yet-complete __when_all_sndr, which is fine -- a trailing return type doesn't
// require completeness until the function is actually called/defined), then __when_all_sndr is
// defined in full (needing when_all_t as a complete, non-dependent `tag` member), then
// when_all_t::operator()'s body is defined out-of-line, after __when_all_sndr is complete --
// matching <__execution/stopped_as_optional.h>'s/<__execution/into_variant.h>'s own ordering,
// for the identical reason.
template <class... _Sndrs>
class __when_all_sndr;

struct when_all_t {
  // [exec.when.all]p2: ill-formed if sizeof...(sndrs) is 0.
  template <sender... _Sndrs>
    requires(sizeof...(_Sndrs) > 0)
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(_Sndrs&&... __sndrs) const
      -> __when_all_sndr<remove_cvref_t<_Sndrs>...>;
};

inline constexpr when_all_t when_all{};

template <class... _Sndrs>
class __when_all_sndr {
public:
  using sender_concept = sender_tag;

  _LIBCPP_NO_UNIQUE_ADDRESS when_all_t tag;
  tuple<_Sndrs...> children;

  // No natural single child to forward attributes from, and nothing in scope through M5
  // queries when_all's own pre-connect attributes -- matches <__execution/just.h>'s/
  // <__execution/read_env.h>'s "no interesting attributes" precedent.
  _LIBCPP_HIDE_FROM_ABI constexpr auto get_env() const noexcept { return env<>{}; }

  template <class _Rcvr>
  _LIBCPP_HIDE_FROM_ABI constexpr auto connect(_Rcvr&& __rcvr) && {
    return __when_all_opstate<remove_cvref_t<_Rcvr>, index_sequence_for<_Sndrs...>, _Sndrs...>(
        std::move(children), std::forward<_Rcvr>(__rcvr));
  }

  template <class _Self, class _Env>
    requires(sender_in<_Sndrs, __fwd_env<remove_cvref_t<_Env>>> && ...)
  _LIBCPP_HIDE_FROM_ABI static consteval auto get_completion_signatures() {
    using __env_t = __fwd_env<remove_cvref_t<_Env>>;
    return __when_all_completion_signatures<__env_t, _Sndrs...>{};
  }
};

template <sender... _Sndrs>
  requires(sizeof...(_Sndrs) > 0)
_LIBCPP_HIDE_FROM_ABI constexpr auto when_all_t::operator()(_Sndrs&&... __sndrs) const
    -> __when_all_sndr<remove_cvref_t<_Sndrs>...> {
  return __when_all_sndr<remove_cvref_t<_Sndrs>...>{{}, tuple<remove_cvref_t<_Sndrs>...>(std::forward<_Sndrs>(__sndrs)...)};
}

} // namespace execution

#endif // _LIBCPP_STD_VER >= 26 && _LIBCPP_HAS_THREADS

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___EXECUTION_WHEN_ALL_H
