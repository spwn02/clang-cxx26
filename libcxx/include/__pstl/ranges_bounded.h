//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___PSTL_RANGES_BOUNDED_H
#define _LIBCPP___PSTL_RANGES_BOUNDED_H

#include <__algorithm/in_in_out_result.h>
#include <__algorithm/in_out_out_result.h>
#include <__algorithm/in_out_result.h>
#include <__config>
#include <__functional/invoke.h>
#include <__iterator/iter_move.h>
#include <__type_traits/common_type.h>
#include <__pstl/backend_fwd.h>
#include <optional>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_PUSH_MACROS
#include <__undef_macros>

#if _LIBCPP_STD_VER >= 26

_LIBCPP_BEGIN_NAMESPACE_STD
namespace __pstl {

template <class _Difference1, class _Difference2>
_LIBCPP_HIDE_FROM_ABI constexpr common_type_t<_Difference1, _Difference2>
__bounded_count(_Difference1 __a, _Difference2 __b) noexcept {
  using _Common = common_type_t<_Difference1, _Difference2>;
  return static_cast<_Common>(__a) < static_cast<_Common>(__b) ? static_cast<_Common>(__a) : static_cast<_Common>(__b);
}

// A bounded set operation may pass over input elements which produce no output
// even after the output range is full. Each backend stops at the first element
// it would have to write beyond the output sentinel.
template <class _ExecutionPolicy>
struct __ranges_bounded_set_union<__default_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _Iter1, class _Sent1, class _Iter2, class _Sent2,
            class _OutIter, class _OutSent, class _Comp, class _Proj1, class _Proj2>
  _LIBCPP_HIDE_FROM_ABI optional<ranges::in_in_out_result<_Iter1, _Iter2, _OutIter>> operator()(
      _Policy&&, _Iter1 __first1, _Sent1 __last1, _Iter2 __first2, _Sent2 __last2,
      _OutIter __result, _OutSent __result_last, _Comp __comp, _Proj1 __proj1, _Proj2 __proj2) const noexcept {
    while (__first1 != __last1 && __first2 != __last2) {
      if (__result == __result_last)
        return ranges::in_in_out_result<_Iter1, _Iter2, _OutIter>{__first1, __first2, __result};
      if (std::invoke(__comp, std::invoke(__proj2, *__first2), std::invoke(__proj1, *__first1))) {
        *__result = *__first2;
        ++__first2;
      } else {
        *__result = *__first1;
        if (!std::invoke(__comp, std::invoke(__proj1, *__first1), std::invoke(__proj2, *__first2)))
          ++__first2;
        ++__first1;
      }
      ++__result;
    }
    while (__first1 != __last1) {
      if (__result == __result_last)
        break;
      *__result = *__first1;
      ++__first1;
      ++__result;
    }
    while (__first2 != __last2) {
      if (__result == __result_last)
        break;
      *__result = *__first2;
      ++__first2;
      ++__result;
    }
    return ranges::in_in_out_result<_Iter1, _Iter2, _OutIter>{__first1, __first2, __result};
  }
};

template <class _ExecutionPolicy>
struct __ranges_bounded_set_intersection<__default_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _Iter1, class _Sent1, class _Iter2, class _Sent2,
            class _OutIter, class _OutSent, class _Comp, class _Proj1, class _Proj2>
  _LIBCPP_HIDE_FROM_ABI optional<ranges::in_in_out_result<_Iter1, _Iter2, _OutIter>> operator()(
      _Policy&&, _Iter1 __first1, _Sent1 __last1, _Iter2 __first2, _Sent2 __last2,
      _OutIter __result, _OutSent __result_last, _Comp __comp, _Proj1 __proj1, _Proj2 __proj2) const noexcept {
    while (__first1 != __last1 && __first2 != __last2) {
      if (std::invoke(__comp, std::invoke(__proj1, *__first1), std::invoke(__proj2, *__first2))) {
        ++__first1;
      } else if (std::invoke(__comp, std::invoke(__proj2, *__first2), std::invoke(__proj1, *__first1))) {
        ++__first2;
      } else {
        if (__result == __result_last)
          break;
        *__result = *__first1;
        ++__first1;
        ++__first2;
        ++__result;
      }
    }
    return ranges::in_in_out_result<_Iter1, _Iter2, _OutIter>{__first1, __first2, __result};
  }
};

// [alg.set.difference]: with M the size of the sorted difference and N = min(M, output size), the result reports
// {last1, first2 + B, result + N} when N == M and {first1 + A, first2 + B, result_last} otherwise, where A and B count the
// copied or skipped elements of each input. The elements paired with an equivalent element of the other range are
// skipped, and an unpaired element of the second range is also skipped if it compares less than the min(N + 1, M)-th
// element of the difference.
template <class _ExecutionPolicy>
struct __ranges_bounded_set_difference<__default_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _Iter1, class _Sent1, class _Iter2, class _Sent2,
            class _OutIter, class _OutSent, class _Comp, class _Proj1, class _Proj2>
  _LIBCPP_HIDE_FROM_ABI optional<ranges::in_in_out_result<_Iter1, _Iter2, _OutIter>> operator()(
      _Policy&&, _Iter1 __first1, _Sent1 __last1, _Iter2 __first2, _Sent2 __last2,
      _OutIter __result, _OutSent __result_last, _Comp __comp, _Proj1 __proj1, _Proj2 __proj2) const noexcept {
    using _Size1 = decltype(__last1 - __first1);
    using _Size2 = decltype(__last2 - __first2);
    const _Size1 __size1 = __last1 - __first1;
    const _Size2 __size2 = __last2 - __first2;
    const auto __capacity = __result_last - __result;

    // First scan: the size M of the difference, the number P of equivalent pairs, the first N difference elements
    // (copied to the output) and the positions of the (N + 1)-th and of the last difference element.
    _Size1 __i1 = 0;
    _Size2 __i2 = 0;
    _Size1 __difference = 0;
    _Size2 __pairs      = 0;
    _Size1 __next_pos   = __size1;
    _Size1 __last_pos   = __size1;
    auto __emit = [&](_Size1 __position) {
      if (static_cast<common_type_t<_Size1, decltype(__capacity)>>(__difference) <
          static_cast<common_type_t<_Size1, decltype(__capacity)>>(__capacity))
        *(__result + __difference) = *(__first1 + __position);
      else if (static_cast<common_type_t<_Size1, decltype(__capacity)>>(__difference) ==
               static_cast<common_type_t<_Size1, decltype(__capacity)>>(__capacity))
        __next_pos = __position;
      __last_pos = __position;
      ++__difference;
    };
    while (__i1 != __size1 && __i2 != __size2) {
      if (std::invoke(__comp, std::invoke(__proj1, *(__first1 + __i1)), std::invoke(__proj2, *(__first2 + __i2)))) {
        __emit(__i1);
        ++__i1;
      } else if (std::invoke(__comp, std::invoke(__proj2, *(__first2 + __i2)), std::invoke(__proj1, *(__first1 + __i1)))) {
        ++__i2;
      } else {
        ++__pairs;
        ++__i1;
        ++__i2;
      }
    }
    for (; __i1 != __size1; ++__i1)
      __emit(__i1);

    const auto __count   = __bounded_count(__difference, __capacity);
    const bool __all     = static_cast<common_type_t<_Size1, decltype(__count)>>(__count) ==
                       static_cast<common_type_t<_Size1, decltype(__count)>>(__difference);
    // The key is the min(N + 1, M)-th element of the difference (none if M == 0).
    const bool __has_key = __difference != 0;
    const _Size1 __key   = __all ? __last_pos : __next_pos;

    // Second scan: the unpaired elements of the second range that compare less than the key.
    _Size2 __skipped = __pairs;
    if (__has_key) {
      _Size1 __j1 = 0;
      _Size2 __j2 = 0;
      while (__j1 != __size1 && __j2 != __size2) {
        if (std::invoke(__comp, std::invoke(__proj1, *(__first1 + __j1)), std::invoke(__proj2, *(__first2 + __j2)))) {
          ++__j1;
        } else if (std::invoke(__comp, std::invoke(__proj2, *(__first2 + __j2)), std::invoke(__proj1, *(__first1 + __j1)))) {
          if (std::invoke(__comp, std::invoke(__proj2, *(__first2 + __j2)), std::invoke(__proj1, *(__first1 + __key))))
            ++__skipped;
          ++__j2;
        } else {
          ++__j1;
          ++__j2;
        }
      }
      for (; __j2 != __size2; ++__j2)
        if (std::invoke(__comp, std::invoke(__proj2, *(__first2 + __j2)), std::invoke(__proj1, *(__first1 + __key))))
          ++__skipped;
    }

    if (__all)
      return ranges::in_in_out_result<_Iter1, _Iter2, _OutIter>{__first1 + __size1, __first2 + __skipped, __result + __count};
    // A counts the copied elements and the elements of the first range paired with an element of the second.
    return ranges::in_in_out_result<_Iter1, _Iter2, _OutIter>{
        __first1 + (__count + __pairs), __first2 + __skipped, __result + __count};
  }
};

template <class _ExecutionPolicy>
struct __ranges_bounded_set_symmetric_difference<__default_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _Iter1, class _Sent1, class _Iter2, class _Sent2,
            class _OutIter, class _OutSent, class _Comp, class _Proj1, class _Proj2>
  _LIBCPP_HIDE_FROM_ABI optional<ranges::in_in_out_result<_Iter1, _Iter2, _OutIter>> operator()(
      _Policy&&, _Iter1 __first1, _Sent1 __last1, _Iter2 __first2, _Sent2 __last2,
      _OutIter __result, _OutSent __result_last, _Comp __comp, _Proj1 __proj1, _Proj2 __proj2) const noexcept {
    while (__first1 != __last1 && __first2 != __last2) {
      if (std::invoke(__comp, std::invoke(__proj1, *__first1), std::invoke(__proj2, *__first2))) {
        if (__result == __result_last)
          return ranges::in_in_out_result<_Iter1, _Iter2, _OutIter>{__first1, __first2, __result};
        *__result = *__first1;
        ++__first1;
        ++__result;
      } else if (std::invoke(__comp, std::invoke(__proj2, *__first2), std::invoke(__proj1, *__first1))) {
        if (__result == __result_last)
          return ranges::in_in_out_result<_Iter1, _Iter2, _OutIter>{__first1, __first2, __result};
        *__result = *__first2;
        ++__first2;
        ++__result;
      } else {
        ++__first1;
        ++__first2;
      }
    }
    while (__first1 != __last1 && __result != __result_last) {
      *__result = *__first1;
      ++__first1;
      ++__result;
    }
    while (__first2 != __last2 && __result != __result_last) {
      *__result = *__first2;
      ++__first2;
      ++__result;
    }
    return ranges::in_in_out_result<_Iter1, _Iter2, _OutIter>{__first1, __first2, __result};
  }
};

template <class _ExecutionPolicy>
struct __ranges_bounded_remove_copy_if<__default_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _Iter, class _Sent, class _OutIter, class _OutSent, class _Pred, class _Proj>
  _LIBCPP_HIDE_FROM_ABI optional<ranges::in_out_result<_Iter, _OutIter>> operator()(
      _Policy&&, _Iter __first, _Sent __last, _OutIter __result, _OutSent __result_last,
      _Pred __pred, _Proj __proj) const noexcept {
    for (; __first != __last; ++__first) {
      if (!std::invoke(__pred, std::invoke(__proj, *__first))) {
        if (__result == __result_last)
          break;
        *__result = *__first;
        ++__result;
      }
    }
    return ranges::in_out_result<_Iter, _OutIter>{__first, __result};
  }
};

template <class _ExecutionPolicy>
struct __ranges_bounded_unique_copy<__default_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _Iter, class _Sent, class _OutIter, class _OutSent, class _Comp, class _Proj>
  _LIBCPP_HIDE_FROM_ABI optional<ranges::in_out_result<_Iter, _OutIter>> operator()(
      _Policy&&, _Iter __first, _Sent __last, _OutIter __result, _OutSent __result_last,
      _Comp __comp, _Proj __proj) const noexcept {
    if (__first == __last || __result == __result_last)
      return ranges::in_out_result<_Iter, _OutIter>{__first, __result};
    *__result = *__first;
    ++__result;
    _Iter __previous = __first;
    ++__first;
    while (__first != __last) {
      if (!std::invoke(__comp, std::invoke(__proj, *__first), std::invoke(__proj, *__previous))) {
        if (__result == __result_last)
          break;
        *__result = *__first;
        ++__result;
      }
      __previous = __first;
      ++__first;
    }
    return ranges::in_out_result<_Iter, _OutIter>{__first, __result};
  }
};


// [alg.copy], [alg.move], [alg.transform], [alg.replace], [alg.reverse], [alg.rotate], [alg.merge], [alg.partitions]:
// the execution-policy overloads of the ranges algorithms with a bounded output process
// N = min(<input size>, <output size>) elements (or stop at the first element that does not fit).
template <class _ExecutionPolicy>
struct __ranges_bounded_copy<__default_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _Iter, class _Sent, class _OutIter, class _OutSent>
  _LIBCPP_HIDE_FROM_ABI optional<ranges::in_out_result<_Iter, _OutIter>>
  operator()(_Policy&&, _Iter __first, _Sent __last, _OutIter __result, _OutSent __result_last) const noexcept {
    auto __n = __pstl::__bounded_count(__last - __first, __result_last - __result);
    for (decltype(__n) __i = 0; __i < __n; ++__i)
      *(__result + __i) = *(__first + __i);
    return ranges::in_out_result<_Iter, _OutIter>{__first + __n, __result + __n};
  }
};

template <class _ExecutionPolicy>
struct __ranges_bounded_copy_n<__default_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _Iter, class _Diff, class _OutIter, class _OutSent>
  _LIBCPP_HIDE_FROM_ABI optional<ranges::in_out_result<_Iter, _OutIter>>
  operator()(_Policy&&, _Iter __first, _Diff __count, _OutIter __result, _OutSent __result_last) const noexcept {
    auto __n = __pstl::__bounded_count(__count > _Diff(0) ? __count : _Diff(0), __result_last - __result);
    for (decltype(__n) __i = 0; __i < __n; ++__i)
      *(__result + __i) = *(__first + __i);
    return ranges::in_out_result<_Iter, _OutIter>{__first + __n, __result + __n};
  }
};

template <class _ExecutionPolicy>
struct __ranges_bounded_move<__default_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _Iter, class _Sent, class _OutIter, class _OutSent>
  _LIBCPP_HIDE_FROM_ABI optional<ranges::in_out_result<_Iter, _OutIter>>
  operator()(_Policy&&, _Iter __first, _Sent __last, _OutIter __result, _OutSent __result_last) const noexcept {
    auto __n = __pstl::__bounded_count(__last - __first, __result_last - __result);
    for (decltype(__n) __i = 0; __i < __n; ++__i)
      *(__result + __i) = ranges::iter_move(__first + __i);
    return ranges::in_out_result<_Iter, _OutIter>{__first + __n, __result + __n};
  }
};

template <class _ExecutionPolicy>
struct __ranges_bounded_copy_if<__default_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _Iter, class _Sent, class _OutIter, class _OutSent, class _Pred, class _Proj>
  _LIBCPP_HIDE_FROM_ABI optional<ranges::in_out_result<_Iter, _OutIter>> operator()(
      _Policy&&, _Iter __first, _Sent __last, _OutIter __result, _OutSent __result_last, _Pred __pred, _Proj __proj)
      const noexcept {
    for (; __first != __last; ++__first) {
      if (std::invoke(__pred, std::invoke(__proj, *__first))) {
        if (__result == __result_last)
          return ranges::in_out_result<_Iter, _OutIter>{__first, __result};
        *__result = *__first;
        ++__result;
      }
    }
    return ranges::in_out_result<_Iter, _OutIter>{__first, __result};
  }
};

template <class _ExecutionPolicy>
struct __ranges_bounded_replace_copy_if<__default_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _Iter, class _Sent, class _OutIter, class _OutSent, class _Pred, class _Tp, class _Proj>
  _LIBCPP_HIDE_FROM_ABI optional<ranges::in_out_result<_Iter, _OutIter>> operator()(
      _Policy&&,
      _Iter __first,
      _Sent __last,
      _OutIter __result,
      _OutSent __result_last,
      _Pred __pred,
      const _Tp& __new_value,
      _Proj __proj) const noexcept {
    auto __n = __pstl::__bounded_count(__last - __first, __result_last - __result);
    for (decltype(__n) __i = 0; __i < __n; ++__i) {
      if (std::invoke(__pred, std::invoke(__proj, *(__first + __i))))
        *(__result + __i) = __new_value;
      else
        *(__result + __i) = *(__first + __i);
    }
    return ranges::in_out_result<_Iter, _OutIter>{__first + __n, __result + __n};
  }
};

template <class _ExecutionPolicy>
struct __ranges_bounded_unary_transform<__default_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _Iter, class _Sent, class _OutIter, class _OutSent, class _Func, class _Proj>
  _LIBCPP_HIDE_FROM_ABI optional<ranges::in_out_result<_Iter, _OutIter>> operator()(
      _Policy&&, _Iter __first, _Sent __last, _OutIter __result, _OutSent __result_last, _Func __func, _Proj __proj)
      const noexcept {
    auto __n = __pstl::__bounded_count(__last - __first, __result_last - __result);
    for (decltype(__n) __i = 0; __i < __n; ++__i)
      *(__result + __i) = std::invoke(__func, std::invoke(__proj, *(__first + __i)));
    return ranges::in_out_result<_Iter, _OutIter>{__first + __n, __result + __n};
  }
};

template <class _ExecutionPolicy>
struct __ranges_bounded_binary_transform<__default_backend_tag, _ExecutionPolicy> {
  template <class _Policy,
            class _Iter1,
            class _Sent1,
            class _Iter2,
            class _Sent2,
            class _OutIter,
            class _OutSent,
            class _Func,
            class _Proj1,
            class _Proj2>
  _LIBCPP_HIDE_FROM_ABI optional<ranges::in_in_out_result<_Iter1, _Iter2, _OutIter>> operator()(
      _Policy&&,
      _Iter1 __first1,
      _Sent1 __last1,
      _Iter2 __first2,
      _Sent2 __last2,
      _OutIter __result,
      _OutSent __result_last,
      _Func __func,
      _Proj1 __proj1,
      _Proj2 __proj2) const noexcept {
    auto __inputs = __pstl::__bounded_count(__last1 - __first1, __last2 - __first2);
    auto __n      = __pstl::__bounded_count(__inputs, __result_last - __result);
    for (decltype(__n) __i = 0; __i < __n; ++__i)
      *(__result + __i) =
          std::invoke(__func, std::invoke(__proj1, *(__first1 + __i)), std::invoke(__proj2, *(__first2 + __i)));
    return ranges::in_in_out_result<_Iter1, _Iter2, _OutIter>{__first1 + __n, __first2 + __n, __result + __n};
  }
};

template <class _ExecutionPolicy>
struct __ranges_bounded_reverse_copy<__default_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _Iter, class _Sent, class _OutIter, class _OutSent>
  _LIBCPP_HIDE_FROM_ABI optional<ranges::in_in_out_result<_Iter, _Iter, _OutIter>>
  operator()(_Policy&&, _Iter __first, _Sent __last, _OutIter __result, _OutSent __result_last) const noexcept {
    auto __size = __last - __first;
    auto __n    = __pstl::__bounded_count(__size, __result_last - __result);
    _Iter __new_first = __first + (__size - __n);
    for (decltype(__n) __i = 0; __i < __n; ++__i)
      *(__result + (__n - 1 - __i)) = *(__new_first + __i);
    return ranges::in_in_out_result<_Iter, _Iter, _OutIter>{__first + __size, __new_first, __result + __n};
  }
};

template <class _ExecutionPolicy>
struct __ranges_bounded_rotate_copy<__default_backend_tag, _ExecutionPolicy> {
  template <class _Policy, class _Iter, class _Sent, class _OutIter, class _OutSent>
  _LIBCPP_HIDE_FROM_ABI optional<ranges::in_in_out_result<_Iter, _Iter, _OutIter>> operator()(
      _Policy&&, _Iter __first, _Iter __middle, _Sent __last, _OutIter __result, _OutSent __result_last) const noexcept {
    auto __size = __last - __first;
    // An empty input has no rotation to describe ([alg.rotate] divides by the size): nothing is copied.
    if (__size == 0)
      return ranges::in_in_out_result<_Iter, _Iter, _OutIter>{__first, __first, __result};
    auto __n      = __pstl::__bounded_count(__size, __result_last - __result);
    auto __offset = __middle - __first;
    for (decltype(__n) __i = 0; __i < __n; ++__i)
      *(__result + __i) = *(__first + ((__i + __offset) % __size));
    if (__n < __last - __middle)
      return ranges::in_in_out_result<_Iter, _Iter, _OutIter>{__middle + __n, __first, __result + __n};
    return ranges::in_in_out_result<_Iter, _Iter, _OutIter>{
        __first + __size, __first + ((__n + __offset) % __size), __result + __n};
  }
};

template <class _ExecutionPolicy>
struct __ranges_bounded_merge<__default_backend_tag, _ExecutionPolicy> {
  template <class _Policy,
            class _Iter1,
            class _Sent1,
            class _Iter2,
            class _Sent2,
            class _OutIter,
            class _OutSent,
            class _Comp,
            class _Proj1,
            class _Proj2>
  _LIBCPP_HIDE_FROM_ABI optional<ranges::in_in_out_result<_Iter1, _Iter2, _OutIter>> operator()(
      _Policy&&,
      _Iter1 __first1,
      _Sent1 __last1,
      _Iter2 __first2,
      _Sent2 __last2,
      _OutIter __result,
      _OutSent __result_last,
      _Comp __comp,
      _Proj1 __proj1,
      _Proj2 __proj2) const noexcept {
    auto __size1 = __last1 - __first1;
    auto __size2 = __last2 - __first2;
    auto __n     = __pstl::__bounded_count(__size1 + __size2, __result_last - __result);
    decltype(__n) __taken1 = 0;
    decltype(__n) __taken2 = 0;
    for (decltype(__n) __i = 0; __i < __n; ++__i) {
      const bool __take_second =
          __taken1 == __size1
              ? true
              : (__taken2 == __size2
                     ? false
                     : static_cast<bool>(std::invoke(
                           __comp, std::invoke(__proj2, *(__first2 + __taken2)), std::invoke(__proj1, *(__first1 + __taken1)))));
      if (__take_second) {
        *(__result + __i) = *(__first2 + __taken2);
        ++__taken2;
      } else {
        *(__result + __i) = *(__first1 + __taken1);
        ++__taken1;
      }
    }
    return ranges::in_in_out_result<_Iter1, _Iter2, _OutIter>{__first1 + __taken1, __first2 + __taken2, __result + __n};
  }
};

template <class _ExecutionPolicy>
struct __ranges_bounded_partition_copy<__default_backend_tag, _ExecutionPolicy> {
  template <class _Policy,
            class _Iter,
            class _Sent,
            class _OutIter1,
            class _OutSent1,
            class _OutIter2,
            class _OutSent2,
            class _Pred,
            class _Proj>
  _LIBCPP_HIDE_FROM_ABI optional<ranges::in_out_out_result<_Iter, _OutIter1, _OutIter2>> operator()(
      _Policy&&,
      _Iter __first,
      _Sent __last,
      _OutIter1 __out_true,
      _OutSent1 __last_true,
      _OutIter2 __out_false,
      _OutSent2 __last_false,
      _Pred __pred,
      _Proj __proj) const noexcept {
    for (; __first != __last; ++__first) {
      if (std::invoke(__pred, std::invoke(__proj, *__first))) {
        if (__out_true == __last_true)
          break;
        *__out_true = *__first;
        ++__out_true;
      } else {
        if (__out_false == __last_false)
          break;
        *__out_false = *__first;
        ++__out_false;
      }
    }
    return ranges::in_out_out_result<_Iter, _OutIter1, _OutIter2>{__first, __out_true, __out_false};
  }
};

} // namespace __pstl
_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_POP_MACROS

#endif // _LIBCPP___PSTL_RANGES_BOUNDED_H
