//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___EXECUTION_DOMAIN_H
#define _LIBCPP___EXECUTION_DOMAIN_H

#include <__config>
#include <__execution/completion_functions.h>
#include <__execution/forwarding_query.h>
#include <__execution/get_env.h>
#include <__execution/get_scheduler.h>
#include <__execution/operation_state.h>
#include <__execution/queryable.h>
#include <__execution/scheduler.h>
#include <__execution/sender.h>
#include <__type_traits/common_type.h>
#include <__type_traits/conditional.h>
#include <__type_traits/decay.h>
#include <__type_traits/is_void.h>
#include <__type_traits/is_same.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/as_const.h>
#include <__utility/declval.h>
#include <__utility/forward.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_PUSH_MACROS
#include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

namespace execution {

template <class... _Domains>
struct indeterminate_domain;

// [exec.domain.default]
struct default_domain {
  // tag_of_t<Sndr>().transform_sender(Tag(), forward<Sndr>(sndr), env) if that expression is well-formed, otherwise
  // static_cast<Sndr>(forward<Sndr>(sndr)); the exception specification is noexcept of the expression chosen.
  template <class _Tag, sender _Sndr, __queryable _Env>
    requires requires(_Sndr&& __sndr, const _Env& __env) {
      tag_of_t<_Sndr>().transform_sender(_Tag(), std::forward<_Sndr>(__sndr), __env);
    }
  _LIBCPP_HIDE_FROM_ABI static constexpr decltype(auto)
  transform_sender(_Tag, _Sndr&& __sndr, const _Env& __env) noexcept(
      noexcept(tag_of_t<_Sndr>().transform_sender(_Tag(), std::forward<_Sndr>(__sndr), __env))) {
    return tag_of_t<_Sndr>().transform_sender(_Tag(), std::forward<_Sndr>(__sndr), __env);
  }

  template <class _Tag, sender _Sndr, __queryable _Env>
    requires(!requires(_Sndr&& __sndr, const _Env& __env) {
      tag_of_t<_Sndr>().transform_sender(_Tag(), std::forward<_Sndr>(__sndr), __env);
    })
  _LIBCPP_HIDE_FROM_ABI static constexpr decltype(auto) transform_sender(_Tag, _Sndr&& __sndr, const _Env&) noexcept(
      noexcept(static_cast<_Sndr>(std::forward<_Sndr>(__sndr)))) {
    return static_cast<_Sndr>(std::forward<_Sndr>(__sndr));
  }

  template <class _Tag, sender _Sndr, class... _Args>
    requires requires(_Sndr&& __sndr, _Args&&... __args) {
      _Tag().apply_sender(std::forward<_Sndr>(__sndr), std::forward<_Args>(__args)...);
    }
  _LIBCPP_HIDE_FROM_ABI static constexpr decltype(auto) apply_sender(_Tag, _Sndr&& __sndr, _Args&&... __args) noexcept(
      noexcept(_Tag().apply_sender(std::forward<_Sndr>(__sndr), std::forward<_Args>(__args)...))) {
    return _Tag().apply_sender(std::forward<_Sndr>(__sndr), std::forward<_Args>(__args)...);
  }
};

// [exec.domain.indeterminate]
template <class... _Domains>
struct indeterminate_domain {
  indeterminate_domain() = default;
  _LIBCPP_HIDE_FROM_ABI constexpr indeterminate_domain(auto&&) noexcept {}

  template <class _Tag, sender _Sndr, __queryable _Env>
  _LIBCPP_HIDE_FROM_ABI static constexpr decltype(auto) transform_sender(_Tag, _Sndr&& __sndr, const _Env& __env) noexcept(
      noexcept(default_domain().transform_sender(_Tag(), std::forward<_Sndr>(__sndr), __env))) {
    // Mandates: for each D in Domains, D().transform_sender(Tag(), forward<Sndr>(sndr), env) is either ill-formed or has
    // the same decayed type as default_domain().transform_sender(Tag(), forward<Sndr>(sndr), env).
    static_assert((__agrees_with_default<_Domains, _Tag, _Sndr, _Env>() && ...),
                  "Mandates: the domains of an indeterminate_domain agree on the type of transform_sender.");
    return default_domain().transform_sender(_Tag(), std::forward<_Sndr>(__sndr), __env);
  }

private:
  template <class _Dom, class _Tag, class _Sndr, class _Env>
  _LIBCPP_HIDE_FROM_ABI static consteval bool __agrees_with_default() {
    if constexpr (requires(_Sndr&& __sndr, const _Env& __env) {
                    _Dom().transform_sender(_Tag(), std::forward<_Sndr>(__sndr), __env);
                  }) {
      return is_same_v<decay_t<decltype(_Dom().transform_sender(
                           _Tag(), std::declval<_Sndr>(), std::declval<const _Env&>()))>,
                       decay_t<decltype(default_domain().transform_sender(
                           _Tag(), std::declval<_Sndr>(), std::declval<const _Env&>()))>>;
    } else {
      return true;
    }
  }
};

template <class _Domains, class... _Ts>
struct __unique_domains;
template <class... _Us>
struct __unique_domains<indeterminate_domain<_Us...>> {
  using type = indeterminate_domain<_Us...>;
};
template <class... _Us, class _Tp, class... _Ts>
struct __unique_domains<indeterminate_domain<_Us...>, _Tp, _Ts...>
    : __unique_domains<conditional_t<(is_same_v<_Tp, _Us> || ...), indeterminate_domain<_Us...>,
                                      indeterminate_domain<_Us..., _Tp>>,
                       _Ts...> {};

} // namespace execution

// [exec.domain.indeterminate]p4: the common type of an indeterminate_domain with another domain.
template <class... _Ds, class... _Us>
struct common_type<execution::indeterminate_domain<_Ds...>, execution::indeterminate_domain<_Us...>> {
  using type = typename execution::__unique_domains<execution::indeterminate_domain<>, _Ds..., _Us...>::type;
};

template <class... _Domains, class _Dom>
struct common_type<execution::indeterminate_domain<_Domains...>, _Dom> {
  using type =
      conditional_t<sizeof...(_Domains) == 0, _Dom,
                    typename common_type<execution::indeterminate_domain<_Domains...>,
                                         execution::indeterminate_domain<_Dom>>::type>;
};

template <class _Dom, class... _Domains>
struct common_type<_Dom, execution::indeterminate_domain<_Domains...>>
    : common_type<execution::indeterminate_domain<_Domains...>, _Dom> {};


namespace execution {

// [exec.snd.expos]: COMMON-DOMAIN(domains...) is common_type_t<decltype(auto(domains))...>() if that expression is
// well-formed, and indeterminate_domain<Ds...>() otherwise, where Ds is the decayed types with duplicates removed.
template <class... _Domains>
_LIBCPP_HIDE_FROM_ABI constexpr auto __common_domain(const _Domains&...) noexcept {
  if constexpr (requires { typename common_type<decay_t<_Domains>...>::type; }) {
    return typename common_type<decay_t<_Domains>...>::type();
  } else {
    return typename __unique_domains<indeterminate_domain<>, decay_t<_Domains>...>::type();
  }
}

// [exec.get.compl.domain]
template <class _Tag>
inline constexpr bool __is_completion_domain_tag =
    is_void_v<_Tag> || is_same_v<_Tag, set_value_t> || is_same_v<_Tag, set_error_t> || is_same_v<_Tag, set_stopped_t>;

template <class _Tag>
struct get_completion_domain_t : forwarding_query_t {
private:
  // Which bullet of [exec.get.compl.domain]p2 applies: 1 is TRY-QUERY(attrs, get_completion_domain<Tag>, envs...), 2 the
  // value domain for the void tag, 3 the domain of the completion scheduler, 4 default_domain for a scheduler given
  // an environment, 0 ill-formed.
  template <class _Attrs, class... _Envs>
  static consteval int __bullet() noexcept {
    if constexpr (!__is_completion_domain_tag<_Tag>) {
      return 0;
    } else if constexpr (requires(const _Attrs& __attrs, const _Envs&... __envs) {
                           execution::__try_query(__attrs, get_completion_domain_t<_Tag>(), __envs...);
                         }) {
      return 1;
    } else if constexpr (is_void_v<_Tag>) {
      // (the requirements are nested so that they are only evaluated for the void tag: for the value tag the
      // expression would be this very operator() and its constraint would depend on itself)
      if constexpr (requires(const _Attrs& __attrs, const _Envs&... __envs) {
                      get_completion_domain_t<set_value_t>()(__attrs, __envs...);
                    }) {
        return 2;
      } else if constexpr (scheduler<_Attrs> && sizeof...(_Envs) > 0) {
        return 4;
      } else {
        return 0;
      }
    } else if constexpr (requires(const _Attrs& __attrs, const _Envs&... __envs) {
                           execution::__try_query(
                               get_completion_scheduler_t<_Tag>()(__attrs, __envs...),
                               get_completion_domain_t<set_value_t>(),
                               __envs...);
                         }) {
      return 3;
    } else if constexpr (scheduler<_Attrs> && sizeof...(_Envs) > 0) {
      return 4;
    } else {
      return 0;
    }
  }

  template <class _Attrs, class... _Envs>
  _LIBCPP_HIDE_FROM_ABI static constexpr auto __domain(const _Attrs& __attrs, const _Envs&... __envs) noexcept {
    constexpr int __b = __bullet<_Attrs, _Envs...>();
    if constexpr (__b == 1) {
      return remove_cvref_t<decltype(execution::__try_query(__attrs, get_completion_domain_t<_Tag>(), __envs...))>();
    } else if constexpr (__b == 2) {
      return remove_cvref_t<decltype(get_completion_domain_t<set_value_t>()(__attrs, __envs...))>();
    } else if constexpr (__b == 3) {
      return remove_cvref_t<decltype(execution::__try_query(
          get_completion_scheduler_t<_Tag>()(__attrs, __envs...), get_completion_domain_t<set_value_t>(), __envs...))>();
    } else {
      return default_domain();
    }
  }

public:
  template <class _Attrs, class... _Envs>
    requires(__bullet<_Attrs, _Envs...>() != 0)
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(const _Attrs& __attrs, const _Envs&... __envs) const noexcept {
    // MANDATE-NOTHROW(D()): the default construction of the domain type is noexcept.
    using _Dom = decltype(__domain(__attrs, __envs...));
    static_assert(noexcept(_Dom()), "Mandates: the default construction of the domain type is noexcept.");
    return __domain(__attrs, __envs...);
  }
};

template <class _Tag = void>
inline constexpr get_completion_domain_t<_Tag> get_completion_domain{};

// [exec.get.domain]
struct get_domain_t : forwarding_query_t {
  template <class _Env>
  _LIBCPP_HIDE_FROM_ABI constexpr auto operator()(const _Env& __env) const noexcept {
    if constexpr (requires { auto(std::as_const(__env).query(*this)); }) {
      // MANDATE-NOTHROW(D()): only the default construction of the domain type is required to be noexcept.
      using _Dom = decltype(auto(std::as_const(__env).query(*this)));
      static_assert(noexcept(_Dom()), "Mandates: the default construction of the domain type is noexcept.");
      return _Dom();
    } else if constexpr (requires {
                           execution::get_completion_domain<set_value_t>(
                               execution::get_scheduler(__env), execution::__hide_sched_fn(__env));
                         }) {
      using _Dom = remove_cvref_t<decltype(execution::get_completion_domain<set_value_t>(
          execution::get_scheduler(__env), execution::__hide_sched_fn(__env)))>;
      static_assert(noexcept(_Dom()), "Mandates: the default construction of the domain type is noexcept.");
      return _Dom();
    } else {
      return default_domain(); // env is evaluated
    }
  }
};
inline constexpr get_domain_t get_domain{};

// [exec.snd.transform]: start-domain and completion-domain(s).
template <class _Env>
_LIBCPP_HIDE_FROM_ABI constexpr auto __start_domain(const _Env& __env) noexcept {
  if constexpr (requires { execution::get_domain(__env); }) {
    return decay_t<decltype(execution::get_domain(__env))>();
  } else {
    return default_domain();
  }
}

template <class _Sndr, class _Env>
_LIBCPP_HIDE_FROM_ABI constexpr auto __completion_domain(_Sndr&& __sndr, const _Env& __env) noexcept {
  if constexpr (requires { execution::get_completion_domain<>(execution::get_env(__sndr), __env); }) {
    return decay_t<decltype(execution::get_completion_domain<>(execution::get_env(__sndr), __env))>();
  } else {
    return default_domain();
  }
}

// transformed-sndr(dom, tag, s): dom.transform_sender(tag, s, env) if that is well-formed, otherwise
// default_domain().transform_sender(tag, s, env).
template <class _Dom, class _Tag, class _Sndr, class _Env>
  requires requires(_Dom __dom, _Tag __tag, _Sndr&& __sndr, const _Env& __env) {
    __dom.transform_sender(__tag, std::forward<_Sndr>(__sndr), __env);
  }
_LIBCPP_HIDE_FROM_ABI constexpr decltype(auto)
__transformed_sndr(_Dom __dom, _Tag __tag, _Sndr&& __sndr, const _Env& __env) noexcept(
    noexcept(__dom.transform_sender(__tag, std::forward<_Sndr>(__sndr), __env))) {
  return __dom.transform_sender(__tag, std::forward<_Sndr>(__sndr), __env);
}

template <class _Dom, class _Tag, class _Sndr, class _Env>
  requires(!requires(_Dom __dom, _Tag __tag, _Sndr&& __sndr, const _Env& __env) {
    __dom.transform_sender(__tag, std::forward<_Sndr>(__sndr), __env);
  })
_LIBCPP_HIDE_FROM_ABI constexpr decltype(auto)
__transformed_sndr(_Dom, _Tag __tag, _Sndr&& __sndr, const _Env& __env) noexcept(
    noexcept(default_domain().transform_sender(__tag, std::forward<_Sndr>(__sndr), __env))) {
  return default_domain().transform_sender(__tag, std::forward<_Sndr>(__sndr), __env);
}

// The domain transform-recurse continues in after one step: start-domain for the start tag, the completion domain of the
// transformed sender otherwise.
template <class _Tag, class _Sndr, class _Env>
struct __next_domain {
  using type = decltype(execution::__completion_domain(std::declval<_Sndr>(), std::declval<const _Env&>()));
};
template <class _Sndr, class _Env>
struct __next_domain<start_t, _Sndr, _Env> {
  using type = decltype(execution::__start_domain(std::declval<const _Env&>()));
};
template <class _Tag, class _Sndr, class _Env>
using __next_domain_t = typename __next_domain<_Tag, _Sndr, _Env>::type;

template <class _Dom, class _Tag, class _Sndr, class _Env>
_LIBCPP_HIDE_FROM_ABI constexpr bool __transform_recurse_nothrow() noexcept {
  using __s2_t = decltype(execution::__transformed_sndr(
      std::declval<_Dom>(), std::declval<_Tag>(), std::declval<_Sndr>(), std::declval<const _Env&>()));
  constexpr bool __step =
      noexcept(execution::__transformed_sndr(std::declval<_Dom>(), std::declval<_Tag>(), std::declval<_Sndr>(), std::declval<const _Env&>()));
  if constexpr (is_same_v<remove_cvref_t<__s2_t>, remove_cvref_t<_Sndr>>) {
    return __step;
  } else {
    return __step && execution::__transform_recurse_nothrow<__next_domain_t<_Tag, __s2_t, _Env>, _Tag, __s2_t, _Env>();
  }
}

// [exec.snd.transform]: transform-recurse(dom, tag, s).
template <class _Dom, class _Tag, class _Sndr, class _Env>
_LIBCPP_HIDE_FROM_ABI constexpr decltype(auto) __transform_recurse(_Dom __dom, _Tag __tag, _Sndr&& __sndr, const _Env& __env) noexcept(
    execution::__transform_recurse_nothrow<_Dom, _Tag, _Sndr, _Env>()) {
  using __s2_t = decltype(execution::__transformed_sndr(__dom, __tag, std::forward<_Sndr>(__sndr), __env));
  if constexpr (is_same_v<remove_cvref_t<__s2_t>, remove_cvref_t<_Sndr>>) {
    return execution::__transformed_sndr(__dom, __tag, std::forward<_Sndr>(__sndr), __env);
  } else {
    decltype(auto) __s2 = execution::__transformed_sndr(__dom, __tag, std::forward<_Sndr>(__sndr), __env);
    if constexpr (is_same_v<_Tag, start_t>) {
      return execution::__transform_recurse(
          execution::__start_domain(__env), __tag, std::forward<decltype(__s2)>(__s2), __env);
    } else {
      return execution::__transform_recurse(
          execution::__completion_domain(__s2, __env), __tag, std::forward<decltype(__s2)>(__s2), __env);
    }
  }
}

template <class _Sndr, class _Env>
using __tmp_sndr_t = decltype(execution::__transform_recurse(
    execution::__completion_domain(std::declval<_Sndr>(), std::declval<const _Env&>()),
    set_value_t{},
    std::declval<_Sndr>(),
    std::declval<const _Env&>()));

template <class _Sndr, class _Env>
_LIBCPP_HIDE_FROM_ABI constexpr bool __transform_sender_nothrow() noexcept {
  return execution::__transform_recurse_nothrow<
             decltype(execution::__completion_domain(std::declval<_Sndr>(), std::declval<const _Env&>())),
             set_value_t, _Sndr, _Env>() &&
         execution::__transform_recurse_nothrow<decltype(execution::__start_domain(std::declval<const _Env&>())),
                                                start_t, __tmp_sndr_t<_Sndr, _Env>, _Env>();
}

// [exec.snd.transform]
template <sender _Sndr, __queryable _Env>
_LIBCPP_HIDE_FROM_ABI constexpr decltype(auto) transform_sender(_Sndr&& __sndr, const _Env& __env) noexcept(
    execution::__transform_sender_nothrow<_Sndr, _Env>()) {
  decltype(auto) __tmp_sndr = execution::__transform_recurse(
      execution::__completion_domain(__sndr, __env), set_value_t{}, std::forward<_Sndr>(__sndr), __env);
  return execution::__transform_recurse(
      execution::__start_domain(__env), start_t{}, std::forward<decltype(__tmp_sndr)>(__tmp_sndr), __env);
}

// [exec.snd.apply]: dom.apply_sender(Tag(), sndr, args...) if that is well-formed, otherwise the same with
// default_domain().
template <class _Domain, class _Tag, sender _Sndr, class... _Args>
  requires requires(_Domain __dom, _Sndr&& __sndr, _Args&&... __args) {
    __dom.apply_sender(_Tag(), std::forward<_Sndr>(__sndr), std::forward<_Args>(__args)...);
  }
_LIBCPP_HIDE_FROM_ABI constexpr decltype(auto) apply_sender(_Domain __dom, _Tag, _Sndr&& __sndr, _Args&&... __args) noexcept(
    noexcept(__dom.apply_sender(_Tag(), std::forward<_Sndr>(__sndr), std::forward<_Args>(__args)...))) {
  return __dom.apply_sender(_Tag(), std::forward<_Sndr>(__sndr), std::forward<_Args>(__args)...);
}

template <class _Domain, class _Tag, sender _Sndr, class... _Args>
  requires(!requires(_Domain __dom, _Sndr&& __sndr, _Args&&... __args) {
            __dom.apply_sender(_Tag(), std::forward<_Sndr>(__sndr), std::forward<_Args>(__args)...);
          }) &&
          requires(_Sndr&& __sndr, _Args&&... __args) {
            default_domain().apply_sender(_Tag(), std::forward<_Sndr>(__sndr), std::forward<_Args>(__args)...);
          }
_LIBCPP_HIDE_FROM_ABI constexpr decltype(auto) apply_sender(_Domain, _Tag, _Sndr&& __sndr, _Args&&... __args) noexcept(
    noexcept(default_domain().apply_sender(_Tag(), std::forward<_Sndr>(__sndr), std::forward<_Args>(__args)...))) {
  return default_domain().apply_sender(_Tag(), std::forward<_Sndr>(__sndr), std::forward<_Args>(__args)...);
}

} // namespace execution

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP___EXECUTION_DOMAIN_H
