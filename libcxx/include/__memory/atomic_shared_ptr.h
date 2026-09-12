//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___MEMORY_ATOMIC_SHARED_PTR_H
#define _LIBCPP___MEMORY_ATOMIC_SHARED_PTR_H

#include <__atomic/atomic_sync.h>
#include <__atomic/check_memory_order.h>
#include <__atomic/memory_order.h>
#include <__config>
#include <__memory/shared_ptr.h>
#include <__utility/move.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 20

// The standard permits these specializations to use locks.  The lock is part
// of the atomic object, which avoids a global lock-table lifetime dependency.
// In particular, keep the reference-count increment performed by load and a
// failed compare_exchange under this lock.  Temporaries containing displaced
// values are destroyed only after releasing it, as [util.smartptr.atomic.general]
// requires.
template <class _Tp>
struct atomic<shared_ptr<_Tp> > {
  using value_type = shared_ptr<_Tp>;

  static constexpr bool is_always_lock_free = false;

private:
  mutable unsigned char __lock_ = 0;
  shared_ptr<_Tp> __value_;

  _LIBCPP_HIDE_FROM_ABI void __lock() const _NOEXCEPT {
    while (__atomic_exchange_n(&__lock_, 1, __ATOMIC_ACQUIRE))
      ;
  }

  _LIBCPP_HIDE_FROM_ABI void __unlock() const _NOEXCEPT { __atomic_store_n(&__lock_, 0, __ATOMIC_RELEASE); }

  _LIBCPP_HIDE_FROM_ABI static bool __equivalent(const shared_ptr<_Tp>& __x, const shared_ptr<_Tp>& __y) _NOEXCEPT {
    return __x.__ptr_ == __y.__ptr_ && (__x.__cntrl_ == __y.__cntrl_ || (!__x.__cntrl_ && !__y.__cntrl_));
  }

public:
  _LIBCPP_HIDE_FROM_ABI constexpr atomic() _NOEXCEPT = default;
  _LIBCPP_HIDE_FROM_ABI constexpr atomic(nullptr_t) _NOEXCEPT : atomic() {}
  _LIBCPP_HIDE_FROM_ABI atomic(shared_ptr<_Tp> __desired) _NOEXCEPT : __value_(std::move(__desired)) {}

  atomic(const atomic&) = delete;
  void operator=(const atomic&) = delete;

  [[__nodiscard__]] _LIBCPP_HIDE_FROM_ABI bool is_lock_free() const _NOEXCEPT { return false; }

  [[__nodiscard__]] _LIBCPP_HIDE_FROM_ABI shared_ptr<_Tp>
  load(memory_order __order = memory_order_seq_cst) const _NOEXCEPT _LIBCPP_CHECK_LOAD_MEMORY_ORDER(__order) {
    __lock();
    shared_ptr<_Tp> __result = __value_;
    __unlock();
    return __result;
  }

  _LIBCPP_HIDE_FROM_ABI operator shared_ptr<_Tp>() const _NOEXCEPT { return load(); }

  _LIBCPP_HIDE_FROM_ABI void
  store(shared_ptr<_Tp> __desired, memory_order __order = memory_order_seq_cst) _NOEXCEPT
      _LIBCPP_CHECK_STORE_MEMORY_ORDER(__order) {
    __lock();
    __value_.swap(__desired);
    __unlock();
  }

  _LIBCPP_HIDE_FROM_ABI void operator=(shared_ptr<_Tp> __desired) _NOEXCEPT { store(std::move(__desired)); }
  _LIBCPP_HIDE_FROM_ABI void operator=(nullptr_t) _NOEXCEPT { store(shared_ptr<_Tp>()); }

  [[__nodiscard__]] _LIBCPP_HIDE_FROM_ABI shared_ptr<_Tp>
  exchange(shared_ptr<_Tp> __desired, memory_order __order = memory_order_seq_cst) _NOEXCEPT {
    (void)__order;
    __lock();
    __value_.swap(__desired);
    __unlock();
    return __desired;
  }

  _LIBCPP_HIDE_FROM_ABI bool compare_exchange_weak(
      shared_ptr<_Tp>& __expected, shared_ptr<_Tp> __desired, memory_order __success, memory_order __failure) _NOEXCEPT
      _LIBCPP_CHECK_EXCHANGE_MEMORY_ORDER(__success, __failure) {
    (void)__success;
    return __compare_exchange(__expected, std::move(__desired));
  }

  _LIBCPP_HIDE_FROM_ABI bool compare_exchange_strong(
      shared_ptr<_Tp>& __expected, shared_ptr<_Tp> __desired, memory_order __success, memory_order __failure) _NOEXCEPT
      _LIBCPP_CHECK_EXCHANGE_MEMORY_ORDER(__success, __failure) {
    (void)__success;
    return __compare_exchange(__expected, std::move(__desired));
  }

  _LIBCPP_HIDE_FROM_ABI bool
  compare_exchange_weak(shared_ptr<_Tp>& __expected, shared_ptr<_Tp> __desired, memory_order __order = memory_order_seq_cst) _NOEXCEPT {
    return compare_exchange_weak(__expected, std::move(__desired), __order, __fail_order(__order));
  }

  _LIBCPP_HIDE_FROM_ABI bool
  compare_exchange_strong(shared_ptr<_Tp>& __expected, shared_ptr<_Tp> __desired, memory_order __order = memory_order_seq_cst) _NOEXCEPT {
    return compare_exchange_strong(__expected, std::move(__desired), __order, __fail_order(__order));
  }

  _LIBCPP_HIDE_FROM_ABI void wait(shared_ptr<_Tp> __old, memory_order __order = memory_order_seq_cst) const _NOEXCEPT
      _LIBCPP_CHECK_WAIT_MEMORY_ORDER(__order) {
    while (__equivalent(load(__order), __old)) {
      __cxx_contention_t __monitor = std::__atomic_monitor_global(this);
      if (!__equivalent(load(__order), __old))
        return;
      std::__atomic_wait_global_table(this, __monitor);
    }
  }

  _LIBCPP_HIDE_FROM_ABI void notify_one() _NOEXCEPT { std::__atomic_notify_one_global_table(this); }
  _LIBCPP_HIDE_FROM_ABI void notify_all() _NOEXCEPT { std::__atomic_notify_all_global_table(this); }

private:
  _LIBCPP_HIDE_FROM_ABI static memory_order __fail_order(memory_order __order) _NOEXCEPT {
    return __order == memory_order_acq_rel ? memory_order_acquire
         : __order == memory_order_release ? memory_order_relaxed
                                           : __order;
  }

  _LIBCPP_HIDE_FROM_ABI bool __compare_exchange(shared_ptr<_Tp>& __expected, shared_ptr<_Tp> __desired) _NOEXCEPT {
    __lock();
    if (__equivalent(__value_, __expected)) {
      __value_.swap(__desired);
      __unlock();
      return true;
    }
    // The count increment is protected by the lock; replacing expected (and
    // therefore releasing its previous value) occurs after the lock is gone.
    shared_ptr<_Tp> __actual = __value_;
    __unlock();
    __expected = std::move(__actual);
    return false;
  }
};

template <class _Tp>
struct atomic<weak_ptr<_Tp> > {
  using value_type = weak_ptr<_Tp>;

  static constexpr bool is_always_lock_free = false;

private:
  mutable unsigned char __lock_ = 0;
  weak_ptr<_Tp> __value_;

  _LIBCPP_HIDE_FROM_ABI void __lock() const _NOEXCEPT {
    while (__atomic_exchange_n(&__lock_, 1, __ATOMIC_ACQUIRE))
      ;
  }
  _LIBCPP_HIDE_FROM_ABI void __unlock() const _NOEXCEPT { __atomic_store_n(&__lock_, 0, __ATOMIC_RELEASE); }
  _LIBCPP_HIDE_FROM_ABI static bool __equivalent(const weak_ptr<_Tp>& __x, const weak_ptr<_Tp>& __y) _NOEXCEPT {
    return __x.__ptr_ == __y.__ptr_ && (__x.__cntrl_ == __y.__cntrl_ || (!__x.__cntrl_ && !__y.__cntrl_));
  }

public:
  _LIBCPP_HIDE_FROM_ABI constexpr atomic() _NOEXCEPT = default;
  _LIBCPP_HIDE_FROM_ABI atomic(weak_ptr<_Tp> __desired) _NOEXCEPT : __value_(std::move(__desired)) {}
  atomic(const atomic&) = delete;
  void operator=(const atomic&) = delete;
  [[__nodiscard__]] _LIBCPP_HIDE_FROM_ABI bool is_lock_free() const _NOEXCEPT { return false; }

  [[__nodiscard__]] _LIBCPP_HIDE_FROM_ABI weak_ptr<_Tp>
  load(memory_order __order = memory_order_seq_cst) const _NOEXCEPT _LIBCPP_CHECK_LOAD_MEMORY_ORDER(__order) {
    __lock();
    weak_ptr<_Tp> __result = __value_;
    __unlock();
    return __result;
  }
  _LIBCPP_HIDE_FROM_ABI operator weak_ptr<_Tp>() const _NOEXCEPT { return load(); }
  _LIBCPP_HIDE_FROM_ABI void
  store(weak_ptr<_Tp> __desired, memory_order __order = memory_order_seq_cst) _NOEXCEPT
      _LIBCPP_CHECK_STORE_MEMORY_ORDER(__order) {
    __lock();
    __value_.swap(__desired);
    __unlock();
  }
  _LIBCPP_HIDE_FROM_ABI void operator=(weak_ptr<_Tp> __desired) _NOEXCEPT { store(std::move(__desired)); }
  [[__nodiscard__]] _LIBCPP_HIDE_FROM_ABI weak_ptr<_Tp>
  exchange(weak_ptr<_Tp> __desired, memory_order __order = memory_order_seq_cst) _NOEXCEPT {
    (void)__order;
    __lock();
    __value_.swap(__desired);
    __unlock();
    return __desired;
  }
  _LIBCPP_HIDE_FROM_ABI bool compare_exchange_weak(
      weak_ptr<_Tp>& __expected, weak_ptr<_Tp> __desired, memory_order __success, memory_order __failure) _NOEXCEPT
      _LIBCPP_CHECK_EXCHANGE_MEMORY_ORDER(__success, __failure) {
    (void)__success;
    return __compare_exchange(__expected, std::move(__desired));
  }
  _LIBCPP_HIDE_FROM_ABI bool compare_exchange_strong(
      weak_ptr<_Tp>& __expected, weak_ptr<_Tp> __desired, memory_order __success, memory_order __failure) _NOEXCEPT
      _LIBCPP_CHECK_EXCHANGE_MEMORY_ORDER(__success, __failure) {
    (void)__success;
    return __compare_exchange(__expected, std::move(__desired));
  }
  _LIBCPP_HIDE_FROM_ABI bool
  compare_exchange_weak(weak_ptr<_Tp>& __expected, weak_ptr<_Tp> __desired, memory_order __order = memory_order_seq_cst) _NOEXCEPT {
    return compare_exchange_weak(__expected, std::move(__desired), __order, __fail_order(__order));
  }
  _LIBCPP_HIDE_FROM_ABI bool
  compare_exchange_strong(weak_ptr<_Tp>& __expected, weak_ptr<_Tp> __desired, memory_order __order = memory_order_seq_cst) _NOEXCEPT {
    return compare_exchange_strong(__expected, std::move(__desired), __order, __fail_order(__order));
  }
  _LIBCPP_HIDE_FROM_ABI void wait(weak_ptr<_Tp> __old, memory_order __order = memory_order_seq_cst) const _NOEXCEPT
      _LIBCPP_CHECK_WAIT_MEMORY_ORDER(__order) {
    while (__equivalent(load(__order), __old)) {
      __cxx_contention_t __monitor = std::__atomic_monitor_global(this);
      if (!__equivalent(load(__order), __old))
        return;
      std::__atomic_wait_global_table(this, __monitor);
    }
  }
  _LIBCPP_HIDE_FROM_ABI void notify_one() _NOEXCEPT { std::__atomic_notify_one_global_table(this); }
  _LIBCPP_HIDE_FROM_ABI void notify_all() _NOEXCEPT { std::__atomic_notify_all_global_table(this); }

private:
  _LIBCPP_HIDE_FROM_ABI static memory_order __fail_order(memory_order __order) _NOEXCEPT {
    return __order == memory_order_acq_rel ? memory_order_acquire
         : __order == memory_order_release ? memory_order_relaxed
                                           : __order;
  }
  _LIBCPP_HIDE_FROM_ABI bool __compare_exchange(weak_ptr<_Tp>& __expected, weak_ptr<_Tp> __desired) _NOEXCEPT {
    __lock();
    if (__equivalent(__value_, __expected)) {
      __value_.swap(__desired);
      __unlock();
      return true;
    }
    weak_ptr<_Tp> __actual = __value_;
    __unlock();
    __expected = std::move(__actual);
    return false;
  }
};

#endif // _LIBCPP_STD_VER >= 20

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___MEMORY_ATOMIC_SHARED_PTR_H
