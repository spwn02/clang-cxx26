// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

/*
    stdlib.h synopsis

Macros:

    EXIT_FAILURE
    EXIT_SUCCESS
    MB_CUR_MAX
    NULL
    RAND_MAX

Types:

    size_t
    div_t
    ldiv_t
    lldiv_t                                                               // C99

double    atof (const char* nptr);
int       atoi (const char* nptr);
long      atol (const char* nptr);
long long atoll(const char* nptr);                                        // C99
double             strtod  (const char* restrict nptr, char** restrict endptr);
float              strtof  (const char* restrict nptr, char** restrict endptr); // C99
long double        strtold (const char* restrict nptr, char** restrict endptr); // C99
long               strtol  (const char* restrict nptr, char** restrict endptr, int base);
long long          strtoll (const char* restrict nptr, char** restrict endptr, int base); // C99
unsigned long      strtoul (const char* restrict nptr, char** restrict endptr, int base);
unsigned long long strtoull(const char* restrict nptr, char** restrict endptr, int base); // C99
int rand(void);
void srand(unsigned int seed);
void* calloc(size_t nmemb, size_t size);
void free(void* ptr);
void* malloc(size_t size);
void* realloc(void* ptr, size_t size);
void abort(void);
int atexit(void (*func)(void));
void exit(int status);
void _Exit(int status);
char* getenv(const char* name);
int system(const char* string);
void* bsearch(const void* key, const void* base, size_t nmemb, size_t size,
              int (*compar)(const void *, const void *));
void qsort(void* base, size_t nmemb, size_t size,
           int (*compar)(const void *, const void *));
int         abs(      int j);
long        abs(     long j);
long long   abs(long long j);                                             // C++0X
long       labs(     long j);
long long llabs(long long j);                                             // C99
div_t     div(      int numer,       int denom);
ldiv_t    div(     long numer,      long denom);
lldiv_t   div(long long numer, long long denom);                          // C++0X
ldiv_t   ldiv(     long numer,      long denom);
lldiv_t lldiv(long long numer, long long denom);                          // C99
int mblen(const char* s, size_t n);
int mbtowc(wchar_t* restrict pwc, const char* restrict s, size_t n);
int wctomb(char* s, wchar_t wchar);
size_t mbstowcs(wchar_t* restrict pwcs, const char* restrict s, size_t n);
size_t wcstombs(char* restrict s, const wchar_t* restrict pwcs, size_t n);
int at_quick_exit(void (*func)(void))                                     // C++11
void quick_exit(int status);                                              // C++11
void *aligned_alloc(size_t alignment, size_t size);                       // C11

*/

#if defined(__cplusplus) && __cplusplus < 201103L && defined(_LIBCPP_USE_FROZEN_CXX03_HEADERS)
#  include <__cxx03/stdlib.h>
#else
#  include <__config>

#  if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#    pragma GCC system_header
#  endif

// The inclusion of the system's <stdlib.h> is intentionally done once outside of any include
// guards because some code expects to be able to include the underlying system header multiple
// times to get different definitions based on the macros that are set before inclusion.
#  if __has_include_next(<stdlib.h>)
#    include_next <stdlib.h>
#  endif

#  if !defined(_LIBCPP_STDLIB_H)
#    define _LIBCPP_STDLIB_H

#    ifdef __cplusplus
extern "C++" {
// abs

#      ifdef abs
#        undef abs
#      endif
#      ifdef labs
#        undef labs
#      endif
#      ifdef llabs
#        undef llabs
#      endif

#      include <__math/abs.h>
using std::__math::abs;


// [support.start.term] and [alg.c.library]: atexit, at_quick_exit, qsort and bsearch are declared both with a
// callback of a function type with C language linkage and with one of a function type with C++ language linkage
// ([dcl.link]: the two are distinct types). The C library declares the first kind; these are the second kind.

_LIBCPP_BEGIN_NAMESPACE_STD

extern "C" {
typedef void __libcpp_c_atexit_handler(void);
typedef int __libcpp_c_compare_pred(const void*, const void*);
}

using ::atexit _LIBCPP_USING_IF_EXISTS;
inline _LIBCPP_HIDE_FROM_ABI int atexit(void (*__func)(void)) _NOEXCEPT {
  return ::atexit(reinterpret_cast<__libcpp_c_atexit_handler*>(__func));
}

#      if !defined(_LIBCPP_CXX03_LANG)
// Some C libraries declare at_quick_exit for a callback with C language linkage, others (glibc) for one with C++
// language linkage; the function is called with whichever kind the C library accepts.
template <class _Target, class _Fn>
struct __libcpp_at_quick_exit_arg {
  typedef _Target type;
};
template <class _Fn>
_LIBCPP_HIDE_FROM_ABI auto __libcpp_at_quick_exit(_Fn* __func, int) _NOEXCEPT
    -> decltype(::at_quick_exit(reinterpret_cast<typename __libcpp_at_quick_exit_arg<__libcpp_c_atexit_handler, _Fn>::type*>(__func))) {
  return ::at_quick_exit(reinterpret_cast<typename __libcpp_at_quick_exit_arg<__libcpp_c_atexit_handler, _Fn>::type*>(__func));
}
template <class _Fn>
_LIBCPP_HIDE_FROM_ABI auto __libcpp_at_quick_exit(_Fn* __func, long) _NOEXCEPT
    -> decltype(::at_quick_exit(reinterpret_cast<typename __libcpp_at_quick_exit_arg<void(void), _Fn>::type*>(__func))) {
  return ::at_quick_exit(reinterpret_cast<typename __libcpp_at_quick_exit_arg<void(void), _Fn>::type*>(__func));
}
inline _LIBCPP_HIDE_FROM_ABI int at_quick_exit(__libcpp_c_atexit_handler* __func) _NOEXCEPT {
  return std::__libcpp_at_quick_exit(__func, 0);
}
inline _LIBCPP_HIDE_FROM_ABI int at_quick_exit(void (*__func)(void)) _NOEXCEPT {
  return std::__libcpp_at_quick_exit(__func, 0);
}
#      endif

using ::qsort _LIBCPP_USING_IF_EXISTS;
inline _LIBCPP_HIDE_FROM_ABI void
qsort(void* __base, size_t __nmemb, size_t __size, int (*__compar)(const void*, const void*)) {
  ::qsort(__base, __nmemb, __size, reinterpret_cast<__libcpp_c_compare_pred*>(__compar));
}

// bsearch returns a pointer with the constness of the array searched; the C library declares one function.
inline _LIBCPP_HIDE_FROM_ABI void*
bsearch(const void* __key, void* __base, size_t __nmemb, size_t __size, __libcpp_c_compare_pred* __compar) {
  return ::bsearch(__key, __base, __nmemb, __size, __compar);
}
inline _LIBCPP_HIDE_FROM_ABI void*
bsearch(const void* __key, void* __base, size_t __nmemb, size_t __size, int (*__compar)(const void*, const void*)) {
  return ::bsearch(__key, __base, __nmemb, __size, reinterpret_cast<__libcpp_c_compare_pred*>(__compar));
}
inline _LIBCPP_HIDE_FROM_ABI const void*
bsearch(const void* __key, const void* __base, size_t __nmemb, size_t __size, __libcpp_c_compare_pred* __compar) {
  return ::bsearch(__key, __base, __nmemb, __size, __compar);
}
inline _LIBCPP_HIDE_FROM_ABI const void*
bsearch(const void* __key, const void* __base, size_t __nmemb, size_t __size, int (*__compar)(const void*, const void*)) {
  return ::bsearch(__key, __base, __nmemb, __size, reinterpret_cast<__libcpp_c_compare_pred*>(__compar));
}

_LIBCPP_END_NAMESPACE_STD

using std::atexit _LIBCPP_USING_IF_EXISTS;
using std::qsort _LIBCPP_USING_IF_EXISTS;
// The C library declares bsearch for a comparison function with C language linkage; [alg.c.library] also
// requires the one with C++ language linkage in the global namespace.
inline _LIBCPP_HIDE_FROM_ABI void* bsearch(
    const void* __key, const void* __base, size_t __nmemb, size_t __size, int (*__compar)(const void*, const void*)) {
  return ::bsearch(__key, __base, __nmemb, __size, reinterpret_cast<std::__libcpp_c_compare_pred*>(__compar));
}

// div

#      ifdef div
#        undef div
#      endif
#      ifdef ldiv
#        undef ldiv
#      endif
#      ifdef lldiv
#        undef lldiv
#      endif

#      if _LIBCPP_STD_VER >= 23
#        include <__math/div.h>
using std::__math::div;
using std::__math::ldiv;
using std::__math::lldiv;
#      else
// MSVCRT already has the correct prototype in <stdlib.h> if __cplusplus is defined
#      if !defined(_LIBCPP_MSVCRT)
inline _LIBCPP_HIDE_FROM_ABI ldiv_t div(long __x, long __y) _NOEXCEPT { return ::ldiv(__x, __y); }
#        if !(defined(__FreeBSD__) && !defined(__LONG_LONG_SUPPORTED))
inline _LIBCPP_HIDE_FROM_ABI lldiv_t div(long long __x, long long __y) _NOEXCEPT { return ::lldiv(__x, __y); }
#        endif
#      endif // _LIBCPP_MSVCRT
#      endif // _LIBCPP_STD_VER >= 23
} // extern "C++"
#    endif   // __cplusplus
#  endif     // _LIBCPP_STDLIB_H

#endif // defined(__cplusplus) && __cplusplus < 201103L && defined(_LIBCPP_USE_FROZEN_CXX03_HEADERS)
