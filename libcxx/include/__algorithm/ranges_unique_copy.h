//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___ALGORITHM_RANGES_UNIQUE_COPY_H
#define _LIBCPP___ALGORITHM_RANGES_UNIQUE_COPY_H

#include <__algorithm/in_out_result.h>
#include <__algorithm/pstl.h>
#include <__pstl/ranges_bounded.h>
#include <__algorithm/iterator_operations.h>
#include <__algorithm/make_projected.h>
#include <__algorithm/unique_copy.h>
#include <__concepts/same_as.h>
#include <__config>
#include <__functional/identity.h>
#include <__functional/invoke.h>
#include <__functional/ranges_operations.h>
#include <__iterator/concepts.h>
#include <__iterator/iterator_traits.h>
#include <__iterator/projected.h>
#include <__ranges/access.h>
#include <__ranges/concepts.h>
#include <__ranges/dangling.h>
#include <__type_traits/is_execution_policy.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/forward.h>
#include <__utility/move.h>
#include <__utility/pair.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_PUSH_MACROS
#include <__undef_macros>

#if _LIBCPP_STD_VER >= 20

_LIBCPP_BEGIN_NAMESPACE_STD

namespace ranges {

template <class _InIter, class _OutIter>
using unique_copy_result = in_out_result<_InIter, _OutIter>;

template <class _InIter, class _OutIter>
concept __can_reread_from_output = (input_iterator<_OutIter> && same_as<iter_value_t<_InIter>, iter_value_t<_OutIter>>);

struct __unique_copy {
  template <class _InIter, class _OutIter>
  static consteval auto __get_algo_tag() {
    if constexpr (forward_iterator<_InIter>) {
      return __unique_copy_tags::__reread_from_input_tag{};
    } else if constexpr (__can_reread_from_output<_InIter, _OutIter>) {
      return __unique_copy_tags::__reread_from_output_tag{};
    } else if constexpr (indirectly_copyable_storable<_InIter, _OutIter>) {
      return __unique_copy_tags::__read_from_tmp_value_tag{};
    }
  }

  template <class _InIter, class _OutIter>
  using __algo_tag_t _LIBCPP_NODEBUG = decltype(__get_algo_tag<_InIter, _OutIter>());

  template <input_iterator _InIter,
            sentinel_for<_InIter> _Sent,
            weakly_incrementable _OutIter,
            class _Proj                                                    = identity,
            indirect_equivalence_relation<projected<_InIter, _Proj>> _Comp = ranges::equal_to>
    requires indirectly_copyable<_InIter, _OutIter> &&
             (forward_iterator<_InIter> ||
              (input_iterator<_OutIter> && same_as<iter_value_t<_InIter>, iter_value_t<_OutIter>>) ||
              indirectly_copyable_storable<_InIter, _OutIter>)
  _LIBCPP_HIDE_FROM_ABI constexpr unique_copy_result<_InIter, _OutIter>
  operator()(_InIter __first, _Sent __last, _OutIter __result, _Comp __comp = {}, _Proj __proj = {}) const {
    auto __ret = std::__unique_copy<_RangeAlgPolicy>(
        std::move(__first),
        std::move(__last),
        std::move(__result),
        std::__make_projected(__comp, __proj),
        __algo_tag_t<_InIter, _OutIter>());
    return {std::move(__ret.first), std::move(__ret.second)};
  }

  template <input_range _Range,
            weakly_incrementable _OutIter,
            class _Proj                                                               = identity,
            indirect_equivalence_relation<projected<iterator_t<_Range>, _Proj>> _Comp = ranges::equal_to>
    requires indirectly_copyable<iterator_t<_Range>, _OutIter> &&
             (forward_iterator<iterator_t<_Range>> ||
              (input_iterator<_OutIter> && same_as<range_value_t<_Range>, iter_value_t<_OutIter>>) ||
              indirectly_copyable_storable<iterator_t<_Range>, _OutIter>)
  _LIBCPP_HIDE_FROM_ABI constexpr unique_copy_result<borrowed_iterator_t<_Range>, _OutIter>
  operator()(_Range&& __range, _OutIter __result, _Comp __comp = {}, _Proj __proj = {}) const {
    auto __ret = std::__unique_copy<_RangeAlgPolicy>(
        ranges::begin(__range),
        ranges::end(__range),
        std::move(__result),
        std::__make_projected(__comp, __proj),
        __algo_tag_t<iterator_t<_Range>, _OutIter>());
    return {std::move(__ret.first), std::move(__ret.second)};
  }

#  if _LIBCPP_HAS_EXPERIMENTAL_PSTL && _LIBCPP_STD_VER >= 26 && !defined(_LIBCPP_FREESTANDING)
  template <class _Ep, random_access_iterator _InIter, sized_sentinel_for<_InIter> _Sent,
            random_access_iterator _OutIter, sized_sentinel_for<_OutIter> _OutSent,
            class _Proj = identity, indirect_equivalence_relation<projected<_InIter, _Proj>> _Comp = ranges::equal_to,
            class _RawPolicy = __remove_cvref_t<_Ep>, enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires indirectly_copyable<_InIter, _OutIter>
  _LIBCPP_HIDE_FROM_ABI unique_copy_result<_InIter, _OutIter>
  operator()(_Ep&& __exec, _InIter __first, _Sent __last, _OutIter __result, _OutSent __result_last,
             _Comp __comp = {}, _Proj __proj = {}) const {
    using _Implementation = __pstl::__dispatch<__pstl::__ranges_bounded_unique_copy,
                                                __pstl::__current_configuration, _RawPolicy>;
    return __pstl::__handle_exception<_Implementation>(std::forward<_Ep>(__exec), std::move(__first),
        std::move(__last), std::move(__result), std::move(__result_last), std::move(__comp), std::move(__proj));
  }

  template <class _Ep, random_access_range _Range, random_access_range _OutRange,
            class _Proj = identity,
            indirect_equivalence_relation<projected<iterator_t<_Range>, _Proj>> _Comp = ranges::equal_to,
            class _RawPolicy = __remove_cvref_t<_Ep>, enable_if_t<is_execution_policy_v<_RawPolicy>, int> = 0>
    requires sized_range<_Range> && sized_range<_OutRange> &&
             indirectly_copyable<iterator_t<_Range>, iterator_t<_OutRange>>
  _LIBCPP_HIDE_FROM_ABI unique_copy_result<borrowed_iterator_t<_Range>, borrowed_iterator_t<_OutRange>>
  operator()(_Ep&& __exec, _Range&& __range, _OutRange&& __result_range,
             _Comp __comp = {}, _Proj __proj = {}) const {
    return (*this)(std::forward<_Ep>(__exec), ranges::begin(__range), ranges::end(__range),
                   ranges::begin(__result_range), ranges::end(__result_range),
                   std::move(__comp), std::move(__proj));
  }
#  endif
};

inline namespace __cpo {
inline constexpr auto unique_copy = __unique_copy{};
} // namespace __cpo
} // namespace ranges

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_STD_VER >= 20

_LIBCPP_POP_MACROS

#endif // _LIBCPP___ALGORITHM_RANGES_UNIQUE_COPY_H
