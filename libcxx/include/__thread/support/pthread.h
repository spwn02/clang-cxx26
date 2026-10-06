// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___THREAD_SUPPORT_PTHREAD_H
#define _LIBCPP___THREAD_SUPPORT_PTHREAD_H

#include <__chrono/convert_to_timespec.h>
#include <__chrono/duration.h>
#include <__config>
#include <ctime>
#include <errno.h>
#include <pthread.h>
#include <sched.h>

#ifdef __MVS__
#  include <__support/ibm/nanosleep.h>
#endif

// Some platforms require <bits/atomic_wide_counter.h> in order for
// PTHREAD_COND_INITIALIZER to be expanded. Normally that would come
// in via <pthread.h>, but it's a non-modular header on those platforms,
// so libc++'s <math.h> usually absorbs atomic_wide_counter.h into the
// module with <math.h> and makes atomic_wide_counter.h invisible.
// Include <math.h> here to work around that.
// This checks wheter a Clang module is built
#if __building_module(std)
#  include <math.h>
#endif

#ifndef _LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

using __libcpp_timespec_t _LIBCPP_NODEBUG = ::timespec;

//
// Mutex
//
typedef pthread_mutex_t __libcpp_mutex_t;
#define _LIBCPP_MUTEX_INITIALIZER PTHREAD_MUTEX_INITIALIZER

typedef pthread_mutex_t __libcpp_recursive_mutex_t;

inline _LIBCPP_HIDE_FROM_ABI int __libcpp_recursive_mutex_init(__libcpp_recursive_mutex_t* __m) {
  pthread_mutexattr_t __attr;
  int __ec = pthread_mutexattr_init(&__attr);
  if (__ec)
    return __ec;
  __ec = pthread_mutexattr_settype(&__attr, PTHREAD_MUTEX_RECURSIVE);
  if (__ec) {
    pthread_mutexattr_destroy(&__attr);
    return __ec;
  }
  __ec = pthread_mutex_init(__m, &__attr);
  if (__ec) {
    pthread_mutexattr_destroy(&__attr);
    return __ec;
  }
  __ec = pthread_mutexattr_destroy(&__attr);
  if (__ec) {
    pthread_mutex_destroy(__m);
    return __ec;
  }
  return 0;
}

_LIBCPP_NO_THREAD_SAFETY_ANALYSIS inline _LIBCPP_HIDE_FROM_ABI int
__libcpp_recursive_mutex_lock(__libcpp_recursive_mutex_t* __m) {
  return pthread_mutex_lock(__m);
}

_LIBCPP_NO_THREAD_SAFETY_ANALYSIS inline _LIBCPP_HIDE_FROM_ABI bool
__libcpp_recursive_mutex_trylock(__libcpp_recursive_mutex_t* __m) {
  return pthread_mutex_trylock(__m) == 0;
}

_LIBCPP_NO_THREAD_SAFETY_ANALYSIS inline _LIBCPP_HIDE_FROM_ABI int
__libcpp_recursive_mutex_unlock(__libcpp_recursive_mutex_t* __m) {
  return pthread_mutex_unlock(__m);
}

inline _LIBCPP_HIDE_FROM_ABI int __libcpp_recursive_mutex_destroy(__libcpp_recursive_mutex_t* __m) {
  return pthread_mutex_destroy(__m);
}

_LIBCPP_NO_THREAD_SAFETY_ANALYSIS inline _LIBCPP_HIDE_FROM_ABI int __libcpp_mutex_lock(__libcpp_mutex_t* __m) {
  return pthread_mutex_lock(__m);
}

_LIBCPP_NO_THREAD_SAFETY_ANALYSIS inline _LIBCPP_HIDE_FROM_ABI bool __libcpp_mutex_trylock(__libcpp_mutex_t* __m) {
  return pthread_mutex_trylock(__m) == 0;
}

_LIBCPP_NO_THREAD_SAFETY_ANALYSIS inline _LIBCPP_HIDE_FROM_ABI int __libcpp_mutex_unlock(__libcpp_mutex_t* __m) {
  return pthread_mutex_unlock(__m);
}

inline _LIBCPP_HIDE_FROM_ABI int __libcpp_mutex_destroy(__libcpp_mutex_t* __m) { return pthread_mutex_destroy(__m); }

//
// Condition Variable
//
typedef pthread_cond_t __libcpp_condvar_t;
#define _LIBCPP_CONDVAR_INITIALIZER PTHREAD_COND_INITIALIZER

inline _LIBCPP_HIDE_FROM_ABI int __libcpp_condvar_signal(__libcpp_condvar_t* __cv) { return pthread_cond_signal(__cv); }

inline _LIBCPP_HIDE_FROM_ABI int __libcpp_condvar_broadcast(__libcpp_condvar_t* __cv) {
  return pthread_cond_broadcast(__cv);
}

_LIBCPP_NO_THREAD_SAFETY_ANALYSIS inline _LIBCPP_HIDE_FROM_ABI int
__libcpp_condvar_wait(__libcpp_condvar_t* __cv, __libcpp_mutex_t* __m) {
  return pthread_cond_wait(__cv, __m);
}

_LIBCPP_NO_THREAD_SAFETY_ANALYSIS inline _LIBCPP_HIDE_FROM_ABI int
__libcpp_condvar_timedwait(__libcpp_condvar_t* __cv, __libcpp_mutex_t* __m, __libcpp_timespec_t* __ts) {
  return pthread_cond_timedwait(__cv, __m, __ts);
}

inline _LIBCPP_HIDE_FROM_ABI int __libcpp_condvar_destroy(__libcpp_condvar_t* __cv) {
  return pthread_cond_destroy(__cv);
}

//
// Execute once
//
typedef pthread_once_t __libcpp_exec_once_flag;
#define _LIBCPP_EXEC_ONCE_INITIALIZER PTHREAD_ONCE_INIT

inline _LIBCPP_HIDE_FROM_ABI int __libcpp_execute_once(__libcpp_exec_once_flag* __flag, void (*__init_routine)()) {
  return pthread_once(__flag, __init_routine);
}

//
// Thread id
//
#if defined(__MVS__)
typedef unsigned long long __libcpp_thread_id;
#else
typedef pthread_t __libcpp_thread_id;
#endif

// Returns non-zero if the thread ids are equal, otherwise 0
inline _LIBCPP_HIDE_FROM_ABI bool __libcpp_thread_id_equal(__libcpp_thread_id __t1, __libcpp_thread_id __t2) {
  return __t1 == __t2;
}

// Returns non-zero if t1 < t2, otherwise 0
inline _LIBCPP_HIDE_FROM_ABI bool __libcpp_thread_id_less(__libcpp_thread_id __t1, __libcpp_thread_id __t2) {
  return __t1 < __t2;
}

//
// Thread
//
#define _LIBCPP_NULL_THREAD ((__libcpp_thread_t()))
typedef pthread_t __libcpp_thread_t;

inline _LIBCPP_HIDE_FROM_ABI __libcpp_thread_id __libcpp_thread_get_id(const __libcpp_thread_t* __t) {
#if defined(__MVS__)
  return __t->__;
#else
  return *__t;
#endif
}

inline _LIBCPP_HIDE_FROM_ABI bool __libcpp_thread_isnull(const __libcpp_thread_t* __t) {
  return __libcpp_thread_get_id(__t) == 0;
}

inline _LIBCPP_HIDE_FROM_ABI int __libcpp_thread_create(__libcpp_thread_t* __t, void* (*__func)(void*), void* __arg) {
  return pthread_create(__t, nullptr, __func, __arg);
}

// [thread.attributes.size]: the stack size is a hint, it is adjusted up to what the platform needs; zero is ignored.
inline _LIBCPP_HIDE_FROM_ABI int
__libcpp_thread_create_with_stack_size(__libcpp_thread_t* __t, void* (*__func)(void*), void* __arg, size_t __stack_size) {
  if (__stack_size == 0)
    return pthread_create(__t, nullptr, __func, __arg);
  pthread_attr_t __attr;
  int __ec = pthread_attr_init(&__attr);
  if (__ec != 0)
    return __ec;
  size_t __min_size = static_cast<size_t>(PTHREAD_STACK_MIN);
  if (__stack_size < __min_size)
    __stack_size = __min_size;
  // a hint: a size the platform rejects (or cannot allocate) is adjusted to the default rather than failing the construction
  __ec = pthread_attr_setstacksize(&__attr, __stack_size);
  if (__ec == 0)
    __ec = pthread_create(__t, &__attr, __func, __arg);
  if (__ec != 0)
    __ec = pthread_create(__t, nullptr, __func, __arg);
  pthread_attr_destroy(&__attr);
  return __ec;
}

// [thread.attributes.hint]: sets the name of the calling thread, a hint: a name that the platform cannot take (too long,
// no support) is silently dropped (the name is truncated to what the platform accepts).
inline _LIBCPP_HIDE_FROM_ABI void __libcpp_thread_set_current_name(const char* __name, size_t __length) {
#if defined(__linux__) || defined(__ANDROID__)
  constexpr size_t __max = 15; // TASK_COMM_LEN - 1
#else
  constexpr size_t __max = 63;
#endif
  char __buffer[__max + 1];
  size_t __n = __length < __max ? __length : __max;
  for (size_t __i = 0; __i != __n; ++__i)
    __buffer[__i] = __name[__i];
  __buffer[__n] = '\0';
#if defined(__linux__) || defined(__ANDROID__) || defined(__GLIBC__)
  (void)pthread_setname_np(pthread_self(), __buffer);
#elif defined(__APPLE__)
  (void)pthread_setname_np(__buffer);
#elif defined(__FreeBSD__) || defined(__OpenBSD__)
  pthread_set_name_np(pthread_self(), __buffer);
#elif defined(__NetBSD__)
  (void)pthread_setname_np(pthread_self(), "%s", __buffer);
#else
  (void)__buffer;
#endif
}

inline _LIBCPP_HIDE_FROM_ABI __libcpp_thread_id __libcpp_thread_get_current_id() {
  const __libcpp_thread_t __current_thread = pthread_self();
  return __libcpp_thread_get_id(&__current_thread);
}

inline _LIBCPP_HIDE_FROM_ABI int __libcpp_thread_join(__libcpp_thread_t* __t) { return pthread_join(*__t, nullptr); }

inline _LIBCPP_HIDE_FROM_ABI int __libcpp_thread_detach(__libcpp_thread_t* __t) { return pthread_detach(*__t); }

inline _LIBCPP_HIDE_FROM_ABI void __libcpp_thread_yield() { sched_yield(); }

inline _LIBCPP_HIDE_FROM_ABI void __libcpp_thread_sleep_for(const chrono::nanoseconds& __ns) {
  __libcpp_timespec_t __ts = std::__convert_to_timespec<__libcpp_timespec_t>(__ns);
  while (nanosleep(&__ts, &__ts) == -1 && errno == EINTR)
    ;
}

//
// Thread local storage
//
#define _LIBCPP_TLS_DESTRUCTOR_CC /* nothing */

typedef pthread_key_t __libcpp_tls_key;

inline _LIBCPP_HIDE_FROM_ABI int __libcpp_tls_create(__libcpp_tls_key* __key, void (*__at_exit)(void*)) {
  return pthread_key_create(__key, __at_exit);
}

inline _LIBCPP_HIDE_FROM_ABI void* __libcpp_tls_get(__libcpp_tls_key __key) { return pthread_getspecific(__key); }

inline _LIBCPP_HIDE_FROM_ABI int __libcpp_tls_set(__libcpp_tls_key __key, void* __p) {
  return pthread_setspecific(__key, __p);
}

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___THREAD_SUPPORT_PTHREAD_H
