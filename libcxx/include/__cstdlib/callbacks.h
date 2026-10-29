//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___CSTDLIB_CALLBACKS_H
#define _LIBCPP___CSTDLIB_CALLBACKS_H

#include <__config>
#include <__type_traits/enable_if.h>
#include <stdlib.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if defined(__cplusplus)

// [support.start.term] and [alg.c.library]: atexit, at_quick_exit, qsort and bsearch are declared both with a
// callback of a function type with C language linkage and with one of a function type with C++ language linkage
// ([dcl.link]: the two are distinct types). The C library declares the first kind; these are the second kind.

_LIBCPP_BEGIN_NAMESPACE_STD

extern "C" {
typedef void __libcpp_c_atexit_handler();
typedef int __libcpp_c_compare_pred(const void*, const void*);
}

using ::atexit _LIBCPP_USING_IF_EXISTS;
inline _LIBCPP_HIDE_FROM_ABI int atexit(void (*__func)()) _NOEXCEPT {
  return ::atexit(reinterpret_cast<__libcpp_c_atexit_handler*>(__func));
}

#      if !defined(_LIBCPP_CXX03_LANG)
// Some C libraries declare at_quick_exit for a callback with C language linkage, others (glibc) for one with C++
// language linkage, and some (Apple's before macOS 15) not at all. The function is called with whichever kind the C
// library accepts, and the overloads below exist only when it accepts one: the call is not qualified, so that a
// missing ::at_quick_exit is a substitution failure (the argument is dependent, and only ADL looks again) and not an
// error where the template is defined.
template <class _Target, class _Fn>
struct __libcpp_at_quick_exit_arg {
  typedef _Target type;
};
template <class _Fn>
_LIBCPP_HIDE_FROM_ABI auto __libcpp_at_quick_exit(_Fn* __func, int) _NOEXCEPT
    -> decltype(at_quick_exit(reinterpret_cast<typename __libcpp_at_quick_exit_arg<__libcpp_c_atexit_handler, _Fn>::type*>(__func))) {
  return at_quick_exit(reinterpret_cast<typename __libcpp_at_quick_exit_arg<__libcpp_c_atexit_handler, _Fn>::type*>(__func));
}
template <class _Fn>
_LIBCPP_HIDE_FROM_ABI auto __libcpp_at_quick_exit(_Fn* __func, long) _NOEXCEPT
    -> decltype(at_quick_exit(reinterpret_cast<typename __libcpp_at_quick_exit_arg<void(), _Fn>::type*>(__func))) {
  return at_quick_exit(reinterpret_cast<typename __libcpp_at_quick_exit_arg<void(), _Fn>::type*>(__func));
}

template <class _Fn, class = void>
inline constexpr bool __libcpp_has_at_quick_exit_for = false;
template <class _Fn>
inline constexpr bool
    __libcpp_has_at_quick_exit_for<_Fn, decltype((void)std::__libcpp_at_quick_exit(static_cast<_Fn*>(nullptr), 0))> =
        true;

// The parameter types are not dependent, so an argument converts implicitly as it does for a plain function (a lambda
// without captures, for example); only the enabling condition depends on the template parameter.
template <class _Fn = __libcpp_c_atexit_handler, __enable_if_t<__libcpp_has_at_quick_exit_for<_Fn>, int> = 0>
inline _LIBCPP_HIDE_FROM_ABI int at_quick_exit(__libcpp_c_atexit_handler* __func) _NOEXCEPT {
  return std::__libcpp_at_quick_exit(static_cast<_Fn*>(__func), 0); // dependent, so that it is not checked when disabled
}
template <class _Fn = void(), __enable_if_t<__libcpp_has_at_quick_exit_for<_Fn>, int> = 0>
inline _LIBCPP_HIDE_FROM_ABI int at_quick_exit(void (*__func)()) _NOEXCEPT {
  return std::__libcpp_at_quick_exit(static_cast<_Fn*>(__func), 0);
}
#      endif

using ::qsort _LIBCPP_USING_IF_EXISTS;
inline _LIBCPP_HIDE_FROM_ABI void
qsort(void* __base, size_t __nmemb, size_t __size, int (*__compar)(const void*, const void*)) {
  ::qsort(__base, __nmemb, __size, reinterpret_cast<__libcpp_c_compare_pred*>(__compar));
}

// The mangling of the MSVC ABI does not tell function types with C and C++ language linkage apart, so the overloads of
// bsearch for a callback of each kind would have the same mangled name (and the compiler rejects the second
// definition). There the overloads for a callback of C++ language linkage are templates, which mangle differently; their
// parameter types are not dependent, so an argument converts as it does for a plain function.
#      if defined(_LIBCPP_ABI_MICROSOFT)
#        define _LIBCPP_CXX_CALLBACK_OVERLOAD template <class _Unused = void>
#      else
#        define _LIBCPP_CXX_CALLBACK_OVERLOAD
#      endif

// bsearch returns a pointer with the constness of the array searched; the C library declares one function.
inline _LIBCPP_HIDE_FROM_ABI void*
bsearch(const void* __key, void* __base, size_t __nmemb, size_t __size, __libcpp_c_compare_pred* __compar) {
  return ::bsearch(__key, __base, __nmemb, __size, __compar);
}
_LIBCPP_CXX_CALLBACK_OVERLOAD
inline _LIBCPP_HIDE_FROM_ABI void*
bsearch(const void* __key, void* __base, size_t __nmemb, size_t __size, int (*__compar)(const void*, const void*)) {
  return ::bsearch(__key, __base, __nmemb, __size, reinterpret_cast<__libcpp_c_compare_pred*>(__compar));
}
inline _LIBCPP_HIDE_FROM_ABI const void*
bsearch(const void* __key, const void* __base, size_t __nmemb, size_t __size, __libcpp_c_compare_pred* __compar) {
  return ::bsearch(__key, __base, __nmemb, __size, __compar);
}
_LIBCPP_CXX_CALLBACK_OVERLOAD
inline _LIBCPP_HIDE_FROM_ABI const void*
bsearch(const void* __key, const void* __base, size_t __nmemb, size_t __size, int (*__compar)(const void*, const void*)) {
  return ::bsearch(__key, __base, __nmemb, __size, reinterpret_cast<__libcpp_c_compare_pred*>(__compar));
}

#      undef _LIBCPP_CXX_CALLBACK_OVERLOAD

_LIBCPP_END_NAMESPACE_STD

#endif // defined(__cplusplus)

#endif // _LIBCPP___CSTDLIB_CALLBACKS_H
