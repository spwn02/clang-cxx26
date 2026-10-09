// -*- C++ -*-
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// ADDITIONAL_COMPILE_FLAGS: -ffreestanding

// P1642R11 [atomics.syn]: <atomic> is freestanding except for
// atomic_signed_lock_free/atomic_unsigned_lock_free (see the two
// atomic-*-lock_free.compile.fail.cpp tests).

#include <atomic>
#include <version>

#ifndef _LIBCPP_FREESTANDING
#  error "-ffreestanding must select libc++ freestanding mode"
#endif

enum class __color { red, green, blue };

void test() {
  std::atomic<bool> __flag_atomic{false};
  std::atomic<int> __int_atomic{0};
  std::atomic<int*> __ptr_atomic{nullptr};
  std::atomic<__color> __enum_atomic{__color::red};
  int __plain_int = 0;
  std::atomic_ref<int> __ref(__plain_int);
  __ref.store(5, std::memory_order_relaxed);
  std::atomic_flag __af = ATOMIC_FLAG_INIT;

  __int_atomic.store(1, std::memory_order_relaxed);
  int __loaded = __int_atomic.load(std::memory_order_acquire);
  __int_atomic.exchange(2, std::memory_order_acq_rel);
  int __expected = 2;
  __int_atomic.compare_exchange_weak(__expected, 3, std::memory_order_seq_cst);
  __int_atomic.compare_exchange_strong(__expected, 4, std::memory_order_seq_cst);
  __int_atomic.fetch_add(1, std::memory_order_relaxed);
  std::atomic_thread_fence(std::memory_order_seq_cst);
  std::atomic_signal_fence(std::memory_order_seq_cst);
  std::kill_dependency(__loaded);
  __af.test_and_set();
  __af.clear();

  static_assert(ATOMIC_BOOL_LOCK_FREE >= 0);
  static_assert(ATOMIC_INT_LOCK_FREE >= 0);
  static_assert(ATOMIC_POINTER_LOCK_FREE >= 0);

  (void)__flag_atomic;
  (void)__ptr_atomic;
  (void)__enum_atomic;
  (void)__loaded;
}
