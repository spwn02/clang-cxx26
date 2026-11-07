//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___TYPE_TRAITS_ALTERNATIVE_TRAITS_H
#define _LIBCPP___TYPE_TRAITS_ALTERNATIVE_TRAITS_H

#include <__config>
#include <__cstddef/nullptr_t.h>
#include <__cstddef/size_t.h>
#include <__meta/metafunction_ids.h>
#include <__type_traits/is_void.h>
#include <__utility/forward.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 29 && __has_feature(pattern_matching)

// [P2688R6] The protocol by which a type describes the alternatives that a
// pattern matching selection can distinguish. The functions here call the
// compiler's metafunction directly: <meta> includes (transitively) the headers
// that specialize alternative_traits.
namespace __alternative_info_detail {
consteval meta::info __constant_of(meta::info __r) {
  return __metafunction(meta::__detail::__metafn_constant_of, __r, ^^__constant_of);
}

consteval bool __is_type(meta::info __r) { return __metafunction(meta::__detail::__metafn_is_type, __r); }

consteval meta::info __reflect_nullptr() {
  return __metafunction(meta::__detail::__metafn_reflect_result, ^^nullptr_t, nullptr, ^^__reflect_nullptr);
}
} // namespace __alternative_info_detail

struct alternative_info {
  meta::info info = {};
  bool empty      = false;

  _LIBCPP_HIDE_FROM_ABI consteval alternative_info() noexcept = default;

  // A reflected constant value is canonicalized; a type is a projectable
  // state and cannot be empty.
  _LIBCPP_HIDE_FROM_ABI consteval alternative_info(meta::info __info, bool __empty = false)
      : info(__info), empty(__empty) {
    if (info != meta::info{}) {
      if (__alternative_info_detail::__is_type(info)) {
        if (empty)
          throw "a typed alternative cannot be empty";
      } else {
        info = __alternative_info_detail::__constant_of(info);
      }
    }
  }
};

template <class _Tp>
struct alternative_traits;

// Raw pointers use the built-in language semantics; this specialization exists
// so that a smart pointer with exactly the same state mapping can reuse it.
template <class _Tp>
struct alternative_traits<_Tp*> {
  static constexpr alternative_info alternatives[] = {
      {__alternative_info_detail::__reflect_nullptr(), /*empty=*/true},
      ^^_Tp,
  };
  static constexpr bool has_residual_states = false;

  enum class state : bool { null = false, value = true };

  // The parameter is templated so nullable library types can reuse this
  // implementation while operating on the actual matching subject.
  template <class _Self>
  _LIBCPP_HIDE_FROM_ABI static constexpr state index(const _Self& __self) noexcept {
    return __self ? state::value : state::null;
  }

  template <state _State, class _Self>
    requires(_State == state::value)
  _LIBCPP_HIDE_FROM_ABI static constexpr decltype(auto) get(_Self&& __self) noexcept {
    return match constexpr (is_void_v<_Tp>) -> decltype(auto) {
      case true => ;
      case false => *std::forward<_Self>(__self);
    };
  }
};

#endif // _LIBCPP_STD_VER >= 29 && __has_feature(pattern_matching)

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___TYPE_TRAITS_ALTERNATIVE_TRAITS_H
