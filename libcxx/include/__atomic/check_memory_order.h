//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___ATOMIC_CHECK_MEMORY_ORDER_H
#define _LIBCPP___ATOMIC_CHECK_MEMORY_ORDER_H

#include <__atomic/memory_order.h>
#include <__config>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

// Compares against the raw (never-deprecated) __legacy_memory_order value
// rather than the memory_order_consume/memory_order::consume name directly,
// so this internal validity check doesn't itself trigger
// -Wdeprecated-declarations (P3475R2 deprecated the public name, not the
// underlying ordering these checks still need to detect).
#define _LIBCPP_CHECK_STORE_MEMORY_ORDER(__m)                                                                          \
  _LIBCPP_DIAGNOSE_WARNING(static_cast<__memory_order_underlying_t>(__m) == __mo_consume ||                            \
                                __m == memory_order_acquire || __m == memory_order_acq_rel,                            \
                           "memory order argument to atomic operation is invalid")

#define _LIBCPP_CHECK_LOAD_MEMORY_ORDER(__m)                                                                           \
  _LIBCPP_DIAGNOSE_WARNING(__m == memory_order_release || __m == memory_order_acq_rel,                                 \
                           "memory order argument to atomic operation is invalid")

#define _LIBCPP_CHECK_EXCHANGE_MEMORY_ORDER(__m, __f)                                                                  \
  _LIBCPP_DIAGNOSE_WARNING(__f == memory_order_release || __f == memory_order_acq_rel,                                 \
                           "memory order argument to atomic operation is invalid")

#define _LIBCPP_CHECK_WAIT_MEMORY_ORDER(__m)                                                                           \
  _LIBCPP_DIAGNOSE_WARNING(__m == memory_order_release || __m == memory_order_acq_rel,                                 \
                           "memory order argument to atomic operation is invalid")

#endif // _LIBCPP___ATOMIC_CHECK_MEMORY_ORDER_H
