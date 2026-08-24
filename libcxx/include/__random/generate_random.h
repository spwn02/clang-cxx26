//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___RANDOM_GENERATE_RANDOM_H
#define _LIBCPP___RANDOM_GENERATE_RANDOM_H

#include <__algorithm/ranges_generate.h>
#include <__concepts/invocable.h>
#include <__config>
#include <__functional/invoke.h>
#include <__functional/reference_wrapper.h>
#include <__iterator/concepts.h>
#include <__iterator/iterator_traits.h>
#include <__iterator/next.h>
#include <__random/uniform_random_bit_generator.h>
#include <__ranges/access.h>
#include <__ranges/concepts.h>
#include <__ranges/dangling.h>
#include <__ranges/subrange.h>
#include <__type_traits/invoke.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/forward.h>
#include <__utility/move.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_PUSH_MACROS
#include <__undef_macros>

#if _LIBCPP_STD_VER >= 26

_LIBCPP_BEGIN_NAMESPACE_STD

namespace ranges {
struct __generate_random {
  // [rand.alg.generate]
  template <class _Rp, class _Gp>
    requires output_range<_Rp, invoke_result_t<_Gp&>> && uniform_random_bit_generator<remove_cvref_t<_Gp>>
  _LIBCPP_HIDE_FROM_ABI constexpr borrowed_iterator_t<_Rp> operator()(_Rp&& __r, _Gp&& __g) const {
    if constexpr (requires { __g.generate_random(std::forward<_Rp>(__r)); }) {
      __g.generate_random(std::forward<_Rp>(__r));
      return ranges::next(ranges::begin(__r), ranges::end(__r));
    } else {
      return ranges::generate(std::forward<_Rp>(__r), std::ref(__g));
    }
  }

  template <class _Gp, output_iterator<invoke_result_t<_Gp&>> _Op, sentinel_for<_Op> _Sp>
    requires uniform_random_bit_generator<remove_cvref_t<_Gp>>
  _LIBCPP_HIDE_FROM_ABI constexpr _Op operator()(_Op __first, _Sp __last, _Gp&& __g) const {
    return (*this)(subrange<_Op, _Sp>(std::move(__first), __last), __g);
  }

  template <class _Rp, class _Gp, class _Dp>
    requires output_range<_Rp, invoke_result_t<_Dp&, _Gp&>> && invocable<_Dp&, _Gp&> &&
             uniform_random_bit_generator<remove_cvref_t<_Gp>>
  _LIBCPP_HIDE_FROM_ABI constexpr borrowed_iterator_t<_Rp> operator()(_Rp&& __r, _Gp&& __g, _Dp&& __d) const {
    if constexpr (requires { __d.generate_random(std::forward<_Rp>(__r), __g); }) {
      __d.generate_random(std::forward<_Rp>(__r), __g);
      return ranges::next(ranges::begin(__r), ranges::end(__r));
    } else {
      return ranges::generate(std::forward<_Rp>(__r), [&__d, &__g] { return std::invoke(__d, __g); });
    }
  }

  template <class _Gp, class _Dp, output_iterator<invoke_result_t<_Dp&, _Gp&>> _Op, sentinel_for<_Op> _Sp>
    requires invocable<_Dp&, _Gp&> && uniform_random_bit_generator<remove_cvref_t<_Gp>>
  _LIBCPP_HIDE_FROM_ABI constexpr _Op operator()(_Op __first, _Sp __last, _Gp&& __g, _Dp&& __d) const {
    return (*this)(subrange<_Op, _Sp>(std::move(__first), __last), __g, __d);
  }
};

inline namespace __cpo {
inline constexpr auto generate_random = __generate_random{};
} // namespace __cpo
} // namespace ranges

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_POP_MACROS

#endif // _LIBCPP___RANDOM_GENERATE_RANDOM_H
