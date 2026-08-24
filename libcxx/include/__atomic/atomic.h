//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___ATOMIC_ATOMIC_H
#define _LIBCPP___ATOMIC_ATOMIC_H

#include <__atomic/atomic_sync.h>
#include <__atomic/atomic_waitable_traits.h>
#include <__atomic/check_memory_order.h>
#include <__atomic/floating_point_helper.h>
#include <__atomic/is_always_lock_free.h>
#include <__atomic/memory_order.h>
#include <__atomic/support.h>
#include <__config>
#include <__cstddef/ptrdiff_t.h>
#include <__memory/addressof.h>
#include <__type_traits/enable_if.h>
#include <__type_traits/is_floating_point.h>
#include <__type_traits/is_function.h>
#include <__type_traits/is_integral.h>
#include <__type_traits/is_nothrow_constructible.h>
#include <__type_traits/is_same.h>
#include <__type_traits/is_scalar.h>
#include <__type_traits/is_trivially_copyable.h>
#include <__type_traits/remove_const.h>
#include <__type_traits/remove_cv.h>
#include <__type_traits/remove_pointer.h>
#include <__type_traits/remove_volatile.h>
#include <__utility/forward.h>
#include <cstring>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

template <class _Tp, bool = is_integral<_Tp>::value && !is_same<_Tp, bool>::value>
struct __atomic_base // false
{
  mutable __cxx_atomic_impl<_Tp> __a_;

  using value_type = _Tp;

#if _LIBCPP_STD_VER >= 17
  static constexpr bool is_always_lock_free = __libcpp_is_always_lock_free<__cxx_atomic_impl<_Tp> >::__value;
#endif

  [[__nodiscard__]] _LIBCPP_HIDE_FROM_ABI bool is_lock_free() const volatile _NOEXCEPT {
    return __cxx_atomic_is_lock_free(sizeof(__cxx_atomic_impl<_Tp>));
  }
  [[__nodiscard__]] _LIBCPP_HIDE_FROM_ABI bool is_lock_free() const _NOEXCEPT {
    return static_cast<__atomic_base const volatile*>(this)->is_lock_free();
  }
  _LIBCPP_HIDE_FROM_ABI void store(_Tp __d, memory_order __m = memory_order_seq_cst) volatile _NOEXCEPT
      _LIBCPP_CHECK_STORE_MEMORY_ORDER(__m) {
    std::__cxx_atomic_store(std::addressof(__a_), __d, __m);
  }
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 void
  store(_Tp __d, memory_order __m = memory_order_seq_cst) _NOEXCEPT _LIBCPP_CHECK_STORE_MEMORY_ORDER(__m) {
    std::__cxx_atomic_store(std::addressof(__a_), __d, __m);
  }
  [[__nodiscard__]] _LIBCPP_HIDE_FROM_ABI _Tp load(memory_order __m = memory_order_seq_cst) const volatile _NOEXCEPT
      _LIBCPP_CHECK_LOAD_MEMORY_ORDER(__m) {
    return std::__cxx_atomic_load(std::addressof(__a_), __m);
  }
<<<<<<< HEAD
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _Tp load(memory_order __m = memory_order_seq_cst) const _NOEXCEPT
=======
  [[__nodiscard__]] _LIBCPP_HIDE_FROM_ABI _Tp load(memory_order __m = memory_order_seq_cst) const _NOEXCEPT
>>>>>>> refs/tags/llvmorg-22.1.8
      _LIBCPP_CHECK_LOAD_MEMORY_ORDER(__m) {
    return std::__cxx_atomic_load(std::addressof(__a_), __m);
  }
  _LIBCPP_HIDE_FROM_ABI operator _Tp() const volatile _NOEXCEPT { return load(); }
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 operator _Tp() const _NOEXCEPT { return load(); }
  _LIBCPP_HIDE_FROM_ABI _Tp exchange(_Tp __d, memory_order __m = memory_order_seq_cst) volatile _NOEXCEPT {
    return std::__cxx_atomic_exchange(std::addressof(__a_), __d, __m);
  }
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _Tp
  exchange(_Tp __d, memory_order __m = memory_order_seq_cst) _NOEXCEPT {
    return std::__cxx_atomic_exchange(std::addressof(__a_), __d, __m);
  }
  _LIBCPP_HIDE_FROM_ABI bool
  compare_exchange_weak(_Tp& __e, _Tp __d, memory_order __s, memory_order __f) volatile _NOEXCEPT
      _LIBCPP_CHECK_EXCHANGE_MEMORY_ORDER(__s, __f) {
    return std::__cxx_atomic_compare_exchange_weak(std::addressof(__a_), std::addressof(__e), __d, __s, __f);
  }
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 bool
  compare_exchange_weak(_Tp& __e, _Tp __d, memory_order __s, memory_order __f) _NOEXCEPT
      _LIBCPP_CHECK_EXCHANGE_MEMORY_ORDER(__s, __f) {
    return std::__cxx_atomic_compare_exchange_weak(std::addressof(__a_), std::addressof(__e), __d, __s, __f);
  }
  _LIBCPP_HIDE_FROM_ABI bool
  compare_exchange_strong(_Tp& __e, _Tp __d, memory_order __s, memory_order __f) volatile _NOEXCEPT
      _LIBCPP_CHECK_EXCHANGE_MEMORY_ORDER(__s, __f) {
    return std::__cxx_atomic_compare_exchange_strong(std::addressof(__a_), std::addressof(__e), __d, __s, __f);
  }
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 bool
  compare_exchange_strong(_Tp& __e, _Tp __d, memory_order __s, memory_order __f) _NOEXCEPT
      _LIBCPP_CHECK_EXCHANGE_MEMORY_ORDER(__s, __f) {
    return std::__cxx_atomic_compare_exchange_strong(std::addressof(__a_), std::addressof(__e), __d, __s, __f);
  }
  _LIBCPP_HIDE_FROM_ABI bool
  compare_exchange_weak(_Tp& __e, _Tp __d, memory_order __m = memory_order_seq_cst) volatile _NOEXCEPT {
    return std::__cxx_atomic_compare_exchange_weak(std::addressof(__a_), std::addressof(__e), __d, __m, __m);
  }
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 bool
  compare_exchange_weak(_Tp& __e, _Tp __d, memory_order __m = memory_order_seq_cst) _NOEXCEPT {
    return std::__cxx_atomic_compare_exchange_weak(std::addressof(__a_), std::addressof(__e), __d, __m, __m);
  }
  _LIBCPP_HIDE_FROM_ABI bool
  compare_exchange_strong(_Tp& __e, _Tp __d, memory_order __m = memory_order_seq_cst) volatile _NOEXCEPT {
    return std::__cxx_atomic_compare_exchange_strong(std::addressof(__a_), std::addressof(__e), __d, __m, __m);
  }
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 bool
  compare_exchange_strong(_Tp& __e, _Tp __d, memory_order __m = memory_order_seq_cst) _NOEXCEPT {
    return std::__cxx_atomic_compare_exchange_strong(std::addressof(__a_), std::addressof(__e), __d, __m, __m);
  }

#if _LIBCPP_STD_VER >= 20
  _LIBCPP_HIDE_FROM_ABI void wait(_Tp __v, memory_order __m = memory_order_seq_cst) const volatile _NOEXCEPT {
    std::__atomic_wait(*this, __v, __m);
  }
<<<<<<< HEAD
  // The consteval branch below deliberately calls this->load() directly instead of going through
  // std::__atomic_wait/__atomic_waitable_traits (kept non-constexpr): atomic_flag.h's own wait()
  // calls std::__atomic_wait before __atomic_waitable_traits<atomic_flag> is declared later in
  // that header, relying on the call not being eagerly instantiated -- which a constexpr
  // __atomic_wait would break (Clang instantiates constexpr function templates eagerly, unlike
  // ordinary ones). Routing only atomic<T>/atomic_ref<T>'s own wait()/notify_*() through a local
  // consteval branch, while leaving the shared std::__atomic_wait/__atomic_notify_* templates
  // untouched, keeps atomic_flag's fragile declaration ordering intact.
  _LIBCPP_AVAILABILITY_SYNC _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 void
  wait(_Tp __v, memory_order __m = memory_order_seq_cst) const _NOEXCEPT {
#  if _LIBCPP_STD_VER >= 26
    if constexpr (is_scalar_v<_Tp>) {
      if consteval {
        // Constant evaluation is single-threaded: nothing else can change the value, and nothing
        // could ever notify a spin here, so looping while equal to __v either returns immediately
        // or never terminates -- surfacing as a constant-evaluation-step-limit failure, matching
        // the paper's "wait-ing for a different value will be a deadlock which should result in
        // constant evaluation failure".
        while (this->load(__m) == __v) {
        }
        return;
      }
    }
#  endif
    std::__atomic_wait(*this, __v, __m);
  }
  _LIBCPP_AVAILABILITY_SYNC _LIBCPP_HIDE_FROM_ABI void notify_one() volatile _NOEXCEPT {
    std::__atomic_notify_one(*this);
  }
  _LIBCPP_AVAILABILITY_SYNC _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 void notify_one() _NOEXCEPT {
#  if _LIBCPP_STD_VER >= 26
    if consteval {
      return; // no-op: constant evaluation is single-threaded, so nothing can be waiting.
    }
#  endif
    std::__atomic_notify_one(*this);
  }
  _LIBCPP_AVAILABILITY_SYNC _LIBCPP_HIDE_FROM_ABI void notify_all() volatile _NOEXCEPT {
    std::__atomic_notify_all(*this);
  }
  _LIBCPP_AVAILABILITY_SYNC _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 void notify_all() _NOEXCEPT {
#  if _LIBCPP_STD_VER >= 26
    if consteval {
      return; // no-op: constant evaluation is single-threaded, so nothing can be waiting.
    }
#  endif
    std::__atomic_notify_all(*this);
  }
=======
  _LIBCPP_HIDE_FROM_ABI void wait(_Tp __v, memory_order __m = memory_order_seq_cst) const _NOEXCEPT {
    std::__atomic_wait(*this, __v, __m);
  }
  _LIBCPP_HIDE_FROM_ABI void notify_one() volatile _NOEXCEPT { std::__atomic_notify_one(*this); }
  _LIBCPP_HIDE_FROM_ABI void notify_one() _NOEXCEPT { std::__atomic_notify_one(*this); }
  _LIBCPP_HIDE_FROM_ABI void notify_all() volatile _NOEXCEPT { std::__atomic_notify_all(*this); }
  _LIBCPP_HIDE_FROM_ABI void notify_all() _NOEXCEPT { std::__atomic_notify_all(*this); }
>>>>>>> refs/tags/llvmorg-22.1.8
#endif //  _LIBCPP_STD_VER >= 20

#if _LIBCPP_STD_VER >= 20
  _LIBCPP_HIDE_FROM_ABI constexpr __atomic_base() noexcept(is_nothrow_default_constructible_v<_Tp>) : __a_(_Tp()) {}
#else
  _LIBCPP_HIDE_FROM_ABI __atomic_base() _NOEXCEPT = default;
#endif

  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR __atomic_base(_Tp __d) _NOEXCEPT : __a_(__d) {}

  __atomic_base(const __atomic_base&) = delete;
};

// atomic<Integral>

template <class _Tp>
struct __atomic_base<_Tp, true> : public __atomic_base<_Tp, false> {
  using __base _LIBCPP_NODEBUG = __atomic_base<_Tp, false>;

  using difference_type = typename __base::value_type;

  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX20 __atomic_base() _NOEXCEPT = default;

  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR __atomic_base(_Tp __d) _NOEXCEPT : __base(__d) {}

  _LIBCPP_HIDE_FROM_ABI _Tp fetch_add(_Tp __op, memory_order __m = memory_order_seq_cst) volatile _NOEXCEPT {
    return std::__cxx_atomic_fetch_add(std::addressof(this->__a_), __op, __m);
  }
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _Tp
  fetch_add(_Tp __op, memory_order __m = memory_order_seq_cst) _NOEXCEPT {
    return std::__cxx_atomic_fetch_add(std::addressof(this->__a_), __op, __m);
  }
  _LIBCPP_HIDE_FROM_ABI _Tp fetch_sub(_Tp __op, memory_order __m = memory_order_seq_cst) volatile _NOEXCEPT {
    return std::__cxx_atomic_fetch_sub(std::addressof(this->__a_), __op, __m);
  }
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _Tp
  fetch_sub(_Tp __op, memory_order __m = memory_order_seq_cst) _NOEXCEPT {
    return std::__cxx_atomic_fetch_sub(std::addressof(this->__a_), __op, __m);
  }
  _LIBCPP_HIDE_FROM_ABI _Tp fetch_and(_Tp __op, memory_order __m = memory_order_seq_cst) volatile _NOEXCEPT {
    return std::__cxx_atomic_fetch_and(std::addressof(this->__a_), __op, __m);
  }
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _Tp
  fetch_and(_Tp __op, memory_order __m = memory_order_seq_cst) _NOEXCEPT {
    return std::__cxx_atomic_fetch_and(std::addressof(this->__a_), __op, __m);
  }
  _LIBCPP_HIDE_FROM_ABI _Tp fetch_or(_Tp __op, memory_order __m = memory_order_seq_cst) volatile _NOEXCEPT {
    return std::__cxx_atomic_fetch_or(std::addressof(this->__a_), __op, __m);
  }
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _Tp
  fetch_or(_Tp __op, memory_order __m = memory_order_seq_cst) _NOEXCEPT {
    return std::__cxx_atomic_fetch_or(std::addressof(this->__a_), __op, __m);
  }
  _LIBCPP_HIDE_FROM_ABI _Tp fetch_xor(_Tp __op, memory_order __m = memory_order_seq_cst) volatile _NOEXCEPT {
    return std::__cxx_atomic_fetch_xor(std::addressof(this->__a_), __op, __m);
  }
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _Tp
  fetch_xor(_Tp __op, memory_order __m = memory_order_seq_cst) _NOEXCEPT {
    return std::__cxx_atomic_fetch_xor(std::addressof(this->__a_), __op, __m);
  }
#if _LIBCPP_STD_VER >= 26
  _LIBCPP_HIDE_FROM_ABI _Tp fetch_max(_Tp __op, memory_order __m = memory_order_seq_cst) volatile _NOEXCEPT {
    return std::__cxx_atomic_fetch_max(std::addressof(this->__a_), __op, __m);
  }
  _LIBCPP_HIDE_FROM_ABI constexpr _Tp fetch_max(_Tp __op, memory_order __m = memory_order_seq_cst) _NOEXCEPT {
    return std::__cxx_atomic_fetch_max(std::addressof(this->__a_), __op, __m);
  }
  _LIBCPP_HIDE_FROM_ABI _Tp fetch_min(_Tp __op, memory_order __m = memory_order_seq_cst) volatile _NOEXCEPT {
    return std::__cxx_atomic_fetch_min(std::addressof(this->__a_), __op, __m);
  }
  _LIBCPP_HIDE_FROM_ABI constexpr _Tp fetch_min(_Tp __op, memory_order __m = memory_order_seq_cst) _NOEXCEPT {
    return std::__cxx_atomic_fetch_min(std::addressof(this->__a_), __op, __m);
  }
#endif // _LIBCPP_STD_VER >= 26

  _LIBCPP_HIDE_FROM_ABI _Tp operator++(int) volatile _NOEXCEPT { return fetch_add(_Tp(1)); }
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _Tp operator++(int) _NOEXCEPT { return fetch_add(_Tp(1)); }
  _LIBCPP_HIDE_FROM_ABI _Tp operator--(int) volatile _NOEXCEPT { return fetch_sub(_Tp(1)); }
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _Tp operator--(int) _NOEXCEPT { return fetch_sub(_Tp(1)); }
  _LIBCPP_HIDE_FROM_ABI _Tp operator++() volatile _NOEXCEPT { return fetch_add(_Tp(1)) + _Tp(1); }
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _Tp operator++() _NOEXCEPT { return fetch_add(_Tp(1)) + _Tp(1); }
  _LIBCPP_HIDE_FROM_ABI _Tp operator--() volatile _NOEXCEPT { return fetch_sub(_Tp(1)) - _Tp(1); }
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _Tp operator--() _NOEXCEPT { return fetch_sub(_Tp(1)) - _Tp(1); }
  _LIBCPP_HIDE_FROM_ABI _Tp operator+=(_Tp __op) volatile _NOEXCEPT { return fetch_add(__op) + __op; }
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _Tp operator+=(_Tp __op) _NOEXCEPT {
    return fetch_add(__op) + __op;
  }
  _LIBCPP_HIDE_FROM_ABI _Tp operator-=(_Tp __op) volatile _NOEXCEPT { return fetch_sub(__op) - __op; }
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _Tp operator-=(_Tp __op) _NOEXCEPT {
    return fetch_sub(__op) - __op;
  }
  _LIBCPP_HIDE_FROM_ABI _Tp operator&=(_Tp __op) volatile _NOEXCEPT { return fetch_and(__op) & __op; }
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _Tp operator&=(_Tp __op) _NOEXCEPT {
    return fetch_and(__op) & __op;
  }
  _LIBCPP_HIDE_FROM_ABI _Tp operator|=(_Tp __op) volatile _NOEXCEPT { return fetch_or(__op) | __op; }
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _Tp operator|=(_Tp __op) _NOEXCEPT {
    return fetch_or(__op) | __op;
  }
  _LIBCPP_HIDE_FROM_ABI _Tp operator^=(_Tp __op) volatile _NOEXCEPT { return fetch_xor(__op) ^ __op; }
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _Tp operator^=(_Tp __op) _NOEXCEPT {
    return fetch_xor(__op) ^ __op;
  }
};

#if _LIBCPP_STD_VER >= 20
// Here we need _IsIntegral because the default template argument is not enough
// e.g  __atomic_base<int> is __atomic_base<int, true>, which inherits from
// __atomic_base<int, false> and the caller of the wait function is
// __atomic_base<int, false>. So specializing __atomic_base<_Tp> does not work
template <class _Tp, bool _IsIntegral>
struct __atomic_waitable_traits<__atomic_base<_Tp, _IsIntegral> > {
  using __value_type _LIBCPP_NODEBUG = _Tp;

  static _LIBCPP_HIDE_FROM_ABI _Tp __atomic_load(const __atomic_base<_Tp, _IsIntegral>& __a, memory_order __order) {
    return __a.load(__order);
  }

  static _LIBCPP_HIDE_FROM_ABI _Tp
  __atomic_load(const volatile __atomic_base<_Tp, _IsIntegral>& __this, memory_order __order) {
    return __this.load(__order);
  }

  static _LIBCPP_HIDE_FROM_ABI const __cxx_atomic_impl<_Tp>*
  __atomic_contention_address(const __atomic_base<_Tp, _IsIntegral>& __a) {
    return std::addressof(__a.__a_);
  }

  static _LIBCPP_HIDE_FROM_ABI const volatile __cxx_atomic_impl<_Tp>*
  __atomic_contention_address(const volatile __atomic_base<_Tp, _IsIntegral>& __this) {
    return std::addressof(__this.__a_);
  }
};

#endif // _LIBCPP_STD_VER >= 20

template <typename _Tp>
struct __check_atomic_mandates {
  using type _LIBCPP_NODEBUG = _Tp;
  static_assert(is_trivially_copyable<_Tp>::value, "std::atomic<T> requires that 'T' be a trivially copyable type");
  // P3323R1: atomic<volatile int> is served by volatile atomic<int> instead -- std::atomic<T> itself is
  // restricted to cv-unqualified T.
  static_assert(is_same<_Tp, __remove_cv_t<_Tp> >::value, "std::atomic<T> requires that 'T' be cv-unqualified");
};

template <class _Tp>
struct atomic : public __atomic_base<typename __check_atomic_mandates<_Tp>::type> {
  using __base _LIBCPP_NODEBUG = __atomic_base<_Tp>;

#if _LIBCPP_STD_VER >= 20
  _LIBCPP_HIDE_FROM_ABI atomic() = default;
#else
  _LIBCPP_HIDE_FROM_ABI atomic() _NOEXCEPT = default;
#endif

  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR atomic(_Tp __d) _NOEXCEPT : __base(__d) {}

  _LIBCPP_HIDE_FROM_ABI _Tp operator=(_Tp __d) volatile _NOEXCEPT {
    __base::store(__d);
    return __d;
  }
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _Tp operator=(_Tp __d) _NOEXCEPT {
    __base::store(__d);
    return __d;
  }

  atomic& operator=(const atomic&)          = delete;
  atomic& operator=(const atomic&) volatile = delete;
};

// atomic<T*>

template <class _Tp>
struct atomic<_Tp*> : public __atomic_base<_Tp*> {
  using __base _LIBCPP_NODEBUG = __atomic_base<_Tp*>;

  using difference_type = ptrdiff_t;

  _LIBCPP_HIDE_FROM_ABI atomic() _NOEXCEPT = default;

  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR atomic(_Tp* __d) _NOEXCEPT : __base(__d) {}

  _LIBCPP_HIDE_FROM_ABI _Tp* operator=(_Tp* __d) volatile _NOEXCEPT {
    __base::store(__d);
    return __d;
  }
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _Tp* operator=(_Tp* __d) _NOEXCEPT {
    __base::store(__d);
    return __d;
  }

  _LIBCPP_HIDE_FROM_ABI _Tp* fetch_add(ptrdiff_t __op, memory_order __m = memory_order_seq_cst) volatile _NOEXCEPT {
    // __atomic_fetch_add accepts function pointers, guard against them.
    static_assert(!is_function<__remove_pointer_t<_Tp> >::value, "Pointer to function isn't allowed");
    return std::__cxx_atomic_fetch_add(std::addressof(this->__a_), __op, __m);
  }

  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _Tp*
  fetch_add(ptrdiff_t __op, memory_order __m = memory_order_seq_cst) _NOEXCEPT {
    // __atomic_fetch_add accepts function pointers, guard against them.
    static_assert(!is_function<__remove_pointer_t<_Tp> >::value, "Pointer to function isn't allowed");
    return std::__cxx_atomic_fetch_add(std::addressof(this->__a_), __op, __m);
  }

  _LIBCPP_HIDE_FROM_ABI _Tp* fetch_sub(ptrdiff_t __op, memory_order __m = memory_order_seq_cst) volatile _NOEXCEPT {
    // __atomic_fetch_add accepts function pointers, guard against them.
    static_assert(!is_function<__remove_pointer_t<_Tp> >::value, "Pointer to function isn't allowed");
    return std::__cxx_atomic_fetch_sub(std::addressof(this->__a_), __op, __m);
  }

  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _Tp*
  fetch_sub(ptrdiff_t __op, memory_order __m = memory_order_seq_cst) _NOEXCEPT {
    // __atomic_fetch_add accepts function pointers, guard against them.
    static_assert(!is_function<__remove_pointer_t<_Tp> >::value, "Pointer to function isn't allowed");
    return std::__cxx_atomic_fetch_sub(std::addressof(this->__a_), __op, __m);
  }

#  if _LIBCPP_STD_VER >= 26
  // Unlike fetch_add/fetch_sub, __atomic_fetch_max/__atomic_fetch_min's ArithAllows is AOEVT_FP only
  // (see SemaChecking.cpp's IsAllowedValueType) -- the compiler builtin rejects pointer operands for
  // these two operations specifically, so this can't route through __cxx_atomic_fetch_max/min like the
  // integral specialization does. [atomics.types.pointer] specifies the computation "as if by max and
  // min algorithms, with the object value and the first parameter as the arguments", i.e. plain
  // operator<, not a NaN-aware comparison -- so a CAS loop with std::max/std::min's own tie-breaking
  // (returns the first argument on equality) suffices.
  _LIBCPP_HIDE_FROM_ABI _Tp* fetch_max(_Tp* __op, memory_order __m = memory_order_seq_cst) volatile _NOEXCEPT {
    _Tp* __old = this->load(memory_order_relaxed);
    _Tp* __new = __old < __op ? __op : __old;
    while (!this->compare_exchange_weak(__old, __new, __m, memory_order_relaxed)) {
      __new = __old < __op ? __op : __old;
    }
    return __old;
  }
  _LIBCPP_HIDE_FROM_ABI constexpr _Tp* fetch_max(_Tp* __op, memory_order __m = memory_order_seq_cst) _NOEXCEPT {
    _Tp* __old = this->load(memory_order_relaxed);
    _Tp* __new = __old < __op ? __op : __old;
    while (!this->compare_exchange_weak(__old, __new, __m, memory_order_relaxed)) {
      __new = __old < __op ? __op : __old;
    }
    return __old;
  }
  _LIBCPP_HIDE_FROM_ABI _Tp* fetch_min(_Tp* __op, memory_order __m = memory_order_seq_cst) volatile _NOEXCEPT {
    _Tp* __old = this->load(memory_order_relaxed);
    _Tp* __new = __op < __old ? __op : __old;
    while (!this->compare_exchange_weak(__old, __new, __m, memory_order_relaxed)) {
      __new = __op < __old ? __op : __old;
    }
    return __old;
  }
  _LIBCPP_HIDE_FROM_ABI constexpr _Tp* fetch_min(_Tp* __op, memory_order __m = memory_order_seq_cst) _NOEXCEPT {
    _Tp* __old = this->load(memory_order_relaxed);
    _Tp* __new = __op < __old ? __op : __old;
    while (!this->compare_exchange_weak(__old, __new, __m, memory_order_relaxed)) {
      __new = __op < __old ? __op : __old;
    }
    return __old;
  }
#  endif // _LIBCPP_STD_VER >= 26

  _LIBCPP_HIDE_FROM_ABI _Tp* operator++(int) volatile _NOEXCEPT { return fetch_add(1); }
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _Tp* operator++(int) _NOEXCEPT { return fetch_add(1); }
  _LIBCPP_HIDE_FROM_ABI _Tp* operator--(int) volatile _NOEXCEPT { return fetch_sub(1); }
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _Tp* operator--(int) _NOEXCEPT { return fetch_sub(1); }
  _LIBCPP_HIDE_FROM_ABI _Tp* operator++() volatile _NOEXCEPT { return fetch_add(1) + 1; }
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _Tp* operator++() _NOEXCEPT { return fetch_add(1) + 1; }
  _LIBCPP_HIDE_FROM_ABI _Tp* operator--() volatile _NOEXCEPT { return fetch_sub(1) - 1; }
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _Tp* operator--() _NOEXCEPT { return fetch_sub(1) - 1; }
  _LIBCPP_HIDE_FROM_ABI _Tp* operator+=(ptrdiff_t __op) volatile _NOEXCEPT { return fetch_add(__op) + __op; }
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _Tp* operator+=(ptrdiff_t __op) _NOEXCEPT {
    return fetch_add(__op) + __op;
  }
  _LIBCPP_HIDE_FROM_ABI _Tp* operator-=(ptrdiff_t __op) volatile _NOEXCEPT { return fetch_sub(__op) - __op; }
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _Tp* operator-=(ptrdiff_t __op) _NOEXCEPT {
    return fetch_sub(__op) - __op;
  }

  atomic& operator=(const atomic&)          = delete;
  atomic& operator=(const atomic&) volatile = delete;
};

#if _LIBCPP_STD_VER >= 20
template <class _Tp>
struct __atomic_waitable_traits<atomic<_Tp> > : __atomic_waitable_traits<__atomic_base<_Tp> > {};

template <class _Tp>
  requires is_floating_point_v<_Tp>
struct atomic<_Tp> : __atomic_base<typename __check_atomic_mandates<_Tp>::type> {
private:
<<<<<<< HEAD
  _LIBCPP_HIDE_FROM_ABI static constexpr bool __is_fp80_long_double() {
    // Only x87-fp80 long double has 64-bit mantissa
    return __LDBL_MANT_DIG__ == 64 && std::is_same_v<_Tp, long double>;
  }

  _LIBCPP_HIDE_FROM_ABI static constexpr bool __has_rmw_builtin() {
#  ifndef _LIBCPP_COMPILER_CLANG_BASED
    return false;
#  else
    // The builtin __cxx_atomic_fetch_add errors during compilation for
    // long double on platforms with fp80 format.
    // For more details, see
    // lib/Sema/SemaChecking.cpp function IsAllowedValueType
    // LLVM Parser does not allow atomicrmw with x86_fp80 type.
    // if (ValType->isSpecificBuiltinType(BuiltinType::LongDouble) &&
    //    &Context.getTargetInfo().getLongDoubleFormat() ==
    //        &llvm::APFloat::x87DoubleExtended())
    // For more info
    // https://github.com/llvm/llvm-project/issues/68602
    // https://reviews.llvm.org/D53965
    return !__is_fp80_long_double();
#  endif
  }

  // Note on constexpr: when __has_rmw_builtin() is true, the builtin_op lambda below forwards straight
  // to __cxx_atomic_fetch_add/max/min (support/c11.h), which are themselves consteval-aware -- so that
  // branch already works at compile time with no changes here. When it's false (e.g. this target's
  // fp80 long double), __rmw_op always takes the CAS loop below instead, both at compile time and at
  // runtime, since __has_rmw_builtin() is a fixed compile-time property of _Tp; that loop already only
  // calls consteval-aware operations (`load`, `compare_exchange_weak`, the pure `_Operation` lambda, and
  // -- for the fp80 clang-bug workaround -- `__cxx_atomic_load_inplace`), so it needs no changes either.
=======
>>>>>>> refs/tags/llvmorg-22.1.8
  template <class _This, class _Operation, class _BuiltinOp>
  _LIBCPP_HIDE_FROM_ABI static _LIBCPP_CONSTEXPR_SINCE_CXX26 _Tp
  __rmw_op(_This&& __self, _Tp __operand, memory_order __m, _Operation __operation, _BuiltinOp __builtin_op) {
    if constexpr (std::__has_rmw_builtin<_Tp>()) {
      return __builtin_op(std::addressof(std::forward<_This>(__self).__a_), __operand, __m);
    } else {
      _Tp __old = __self.load(memory_order_relaxed);
      _Tp __new = __operation(__old, __operand);
      while (!__self.compare_exchange_weak(__old, __new, __m, memory_order_relaxed)) {
#  ifdef _LIBCPP_COMPILER_CLANG_BASED
        if constexpr (std::__is_fp80_long_double<_Tp>()) {
          // https://llvm.org/PR47978
          // clang bug: __old is not updated on failure for atomic<long double>::compare_exchange_weak
          // Note __old = __self.load(memory_order_relaxed) will not work
          std::__cxx_atomic_load_inplace(std::addressof(__self.__a_), std::addressof(__old), memory_order_relaxed);
        }
#  endif
        __new = __operation(__old, __operand);
      }
      return __old;
    }
  }

  template <class _This>
  _LIBCPP_HIDE_FROM_ABI static _LIBCPP_CONSTEXPR_SINCE_CXX26 _Tp
  __fetch_add(_This&& __self, _Tp __operand, memory_order __m) {
    auto __builtin_op = [](auto __a, auto __builtin_operand, auto __order) {
      return std::__cxx_atomic_fetch_add(__a, __builtin_operand, __order);
    };
    auto __plus = [](auto __a, auto __b) { return __a + __b; };
    return __rmw_op(std::forward<_This>(__self), __operand, __m, __plus, __builtin_op);
  }

  template <class _This>
  _LIBCPP_HIDE_FROM_ABI static _LIBCPP_CONSTEXPR_SINCE_CXX26 _Tp
  __fetch_sub(_This&& __self, _Tp __operand, memory_order __m) {
    auto __builtin_op = [](auto __a, auto __builtin_operand, auto __order) {
      return std::__cxx_atomic_fetch_sub(__a, __builtin_operand, __order);
    };
    auto __minus = [](auto __a, auto __b) { return __a - __b; };
    return __rmw_op(std::forward<_This>(__self), __operand, __m, __minus, __builtin_op);
  }

#  if _LIBCPP_STD_VER >= 26
  // As if by fmaximum_num/fminimum_num: unlike a plain `<`-based max/min, NaN never propagates into the
  // stored value. If both operands are NaN, or exactly one is, an unspecified NaN value (either operand)
  // is stored, matching [atomics.types.float]'s "unspecified which" wording.
  _LIBCPP_HIDE_FROM_ABI static constexpr _Tp __maximum_num(_Tp __a, _Tp __b) {
    if (__builtin_isnan(__a))
      return __b;
    if (__builtin_isnan(__b))
      return __a;
    return __a < __b ? __b : __a;
  }
  _LIBCPP_HIDE_FROM_ABI static constexpr _Tp __minimum_num(_Tp __a, _Tp __b) {
    if (__builtin_isnan(__a))
      return __b;
    if (__builtin_isnan(__b))
      return __a;
    return __b < __a ? __b : __a;
  }

  template <class _This>
  _LIBCPP_HIDE_FROM_ABI static constexpr _Tp __fetch_max(_This&& __self, _Tp __operand, memory_order __m) {
    auto __builtin_op = [](auto __a, auto __builtin_operand, auto __order) {
      return std::__cxx_atomic_fetch_max(__a, __builtin_operand, __order);
    };
    return __rmw_op(std::forward<_This>(__self), __operand, __m, __maximum_num, __builtin_op);
  }

  template <class _This>
  _LIBCPP_HIDE_FROM_ABI static constexpr _Tp __fetch_min(_This&& __self, _Tp __operand, memory_order __m) {
    auto __builtin_op = [](auto __a, auto __builtin_operand, auto __order) {
      return std::__cxx_atomic_fetch_min(__a, __builtin_operand, __order);
    };
    return __rmw_op(std::forward<_This>(__self), __operand, __m, __minimum_num, __builtin_op);
  }
#  endif // _LIBCPP_STD_VER >= 26

public:
  using __base _LIBCPP_NODEBUG = __atomic_base<_Tp>;
  using value_type             = _Tp;
  using difference_type        = value_type;

  _LIBCPP_HIDE_FROM_ABI constexpr atomic() noexcept = default;
  _LIBCPP_HIDE_FROM_ABI constexpr atomic(_Tp __d) noexcept : __base(__d) {}

  atomic(const atomic&)                     = delete;
  atomic& operator=(const atomic&)          = delete;
  atomic& operator=(const atomic&) volatile = delete;

  _LIBCPP_HIDE_FROM_ABI _Tp operator=(_Tp __d) volatile noexcept
    requires __base::is_always_lock_free
  {
    __base::store(__d);
    return __d;
  }
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _Tp operator=(_Tp __d) noexcept {
    __base::store(__d);
    return __d;
  }

  _LIBCPP_HIDE_FROM_ABI _Tp fetch_add(_Tp __op, memory_order __m = memory_order_seq_cst) volatile noexcept
    requires __base::is_always_lock_free
  {
    return __fetch_add(*this, __op, __m);
  }

  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _Tp
  fetch_add(_Tp __op, memory_order __m = memory_order_seq_cst) noexcept {
    return __fetch_add(*this, __op, __m);
  }

  _LIBCPP_HIDE_FROM_ABI _Tp fetch_sub(_Tp __op, memory_order __m = memory_order_seq_cst) volatile noexcept
    requires __base::is_always_lock_free
  {
    return __fetch_sub(*this, __op, __m);
  }

  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _Tp
  fetch_sub(_Tp __op, memory_order __m = memory_order_seq_cst) noexcept {
    return __fetch_sub(*this, __op, __m);
  }

#  if _LIBCPP_STD_VER >= 26
  _LIBCPP_HIDE_FROM_ABI _Tp fetch_max(_Tp __op, memory_order __m = memory_order_seq_cst) volatile noexcept
    requires __base::is_always_lock_free
  {
    return __fetch_max(*this, __op, __m);
  }

  _LIBCPP_HIDE_FROM_ABI constexpr _Tp fetch_max(_Tp __op, memory_order __m = memory_order_seq_cst) noexcept {
    return __fetch_max(*this, __op, __m);
  }

  _LIBCPP_HIDE_FROM_ABI _Tp fetch_min(_Tp __op, memory_order __m = memory_order_seq_cst) volatile noexcept
    requires __base::is_always_lock_free
  {
    return __fetch_min(*this, __op, __m);
  }

  _LIBCPP_HIDE_FROM_ABI constexpr _Tp fetch_min(_Tp __op, memory_order __m = memory_order_seq_cst) noexcept {
    return __fetch_min(*this, __op, __m);
  }
#  endif // _LIBCPP_STD_VER >= 26

  _LIBCPP_HIDE_FROM_ABI _Tp operator+=(_Tp __op) volatile noexcept
    requires __base::is_always_lock_free
  {
    return fetch_add(__op) + __op;
  }

  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _Tp operator+=(_Tp __op) noexcept {
    return fetch_add(__op) + __op;
  }

  _LIBCPP_HIDE_FROM_ABI _Tp operator-=(_Tp __op) volatile noexcept
    requires __base::is_always_lock_free
  {
    return fetch_sub(__op) - __op;
  }

  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _Tp operator-=(_Tp __op) noexcept {
    return fetch_sub(__op) - __op;
  }
};

#endif // _LIBCPP_STD_VER >= 20

// atomic_is_lock_free

template <class _Tp>
[[__nodiscard__]] _LIBCPP_HIDE_FROM_ABI bool atomic_is_lock_free(const volatile atomic<_Tp>* __o) _NOEXCEPT {
  return __o->is_lock_free();
}

template <class _Tp>
[[__nodiscard__]] _LIBCPP_HIDE_FROM_ABI bool atomic_is_lock_free(const atomic<_Tp>* __o) _NOEXCEPT {
  return __o->is_lock_free();
}

// atomic_init

template <class _Tp>
_LIBCPP_DEPRECATED_IN_CXX20 _LIBCPP_HIDE_FROM_ABI void
atomic_init(volatile atomic<_Tp>* __o, typename atomic<_Tp>::value_type __d) _NOEXCEPT {
  std::__cxx_atomic_init(std::addressof(__o->__a_), __d);
}

template <class _Tp>
_LIBCPP_DEPRECATED_IN_CXX20 _LIBCPP_HIDE_FROM_ABI void
atomic_init(atomic<_Tp>* __o, typename atomic<_Tp>::value_type __d) _NOEXCEPT {
  std::__cxx_atomic_init(std::addressof(__o->__a_), __d);
}

// atomic_store

template <class _Tp>
_LIBCPP_HIDE_FROM_ABI void atomic_store(volatile atomic<_Tp>* __o, typename atomic<_Tp>::value_type __d) _NOEXCEPT {
  __o->store(__d);
}

template <class _Tp>
_LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 void
atomic_store(atomic<_Tp>* __o, typename atomic<_Tp>::value_type __d) _NOEXCEPT {
  __o->store(__d);
}

// atomic_store_explicit

template <class _Tp>
_LIBCPP_HIDE_FROM_ABI void
atomic_store_explicit(volatile atomic<_Tp>* __o, typename atomic<_Tp>::value_type __d, memory_order __m) _NOEXCEPT
    _LIBCPP_CHECK_STORE_MEMORY_ORDER(__m) {
  __o->store(__d, __m);
}

template <class _Tp>
_LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 void
atomic_store_explicit(atomic<_Tp>* __o, typename atomic<_Tp>::value_type __d, memory_order __m) _NOEXCEPT
    _LIBCPP_CHECK_STORE_MEMORY_ORDER(__m) {
  __o->store(__d, __m);
}

// atomic_load

template <class _Tp>
[[__nodiscard__]] _LIBCPP_HIDE_FROM_ABI _Tp atomic_load(const volatile atomic<_Tp>* __o) _NOEXCEPT {
  return __o->load();
}

template <class _Tp>
<<<<<<< HEAD
_LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _Tp atomic_load(const atomic<_Tp>* __o) _NOEXCEPT {
=======
[[__nodiscard__]] _LIBCPP_HIDE_FROM_ABI _Tp atomic_load(const atomic<_Tp>* __o) _NOEXCEPT {
>>>>>>> refs/tags/llvmorg-22.1.8
  return __o->load();
}

// atomic_load_explicit

template <class _Tp>
[[__nodiscard__]] _LIBCPP_HIDE_FROM_ABI _Tp
atomic_load_explicit(const volatile atomic<_Tp>* __o, memory_order __m) _NOEXCEPT _LIBCPP_CHECK_LOAD_MEMORY_ORDER(__m) {
  return __o->load(__m);
}

template <class _Tp>
<<<<<<< HEAD
_LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _Tp
atomic_load_explicit(const atomic<_Tp>* __o, memory_order __m) _NOEXCEPT _LIBCPP_CHECK_LOAD_MEMORY_ORDER(__m) {
=======
[[__nodiscard__]] _LIBCPP_HIDE_FROM_ABI _Tp atomic_load_explicit(const atomic<_Tp>* __o, memory_order __m) _NOEXCEPT
    _LIBCPP_CHECK_LOAD_MEMORY_ORDER(__m) {
>>>>>>> refs/tags/llvmorg-22.1.8
  return __o->load(__m);
}

// atomic_exchange

template <class _Tp>
_LIBCPP_HIDE_FROM_ABI _Tp atomic_exchange(volatile atomic<_Tp>* __o, typename atomic<_Tp>::value_type __d) _NOEXCEPT {
  return __o->exchange(__d);
}

template <class _Tp>
_LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _Tp
atomic_exchange(atomic<_Tp>* __o, typename atomic<_Tp>::value_type __d) _NOEXCEPT {
  return __o->exchange(__d);
}

// atomic_exchange_explicit

template <class _Tp>
_LIBCPP_HIDE_FROM_ABI _Tp
atomic_exchange_explicit(volatile atomic<_Tp>* __o, typename atomic<_Tp>::value_type __d, memory_order __m) _NOEXCEPT {
  return __o->exchange(__d, __m);
}

template <class _Tp>
_LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _Tp
atomic_exchange_explicit(atomic<_Tp>* __o, typename atomic<_Tp>::value_type __d, memory_order __m) _NOEXCEPT {
  return __o->exchange(__d, __m);
}

// atomic_compare_exchange_weak

template <class _Tp>
_LIBCPP_HIDE_FROM_ABI bool atomic_compare_exchange_weak(
    volatile atomic<_Tp>* __o, typename atomic<_Tp>::value_type* __e, typename atomic<_Tp>::value_type __d) _NOEXCEPT {
  return __o->compare_exchange_weak(*__e, __d);
}

template <class _Tp>
_LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 bool atomic_compare_exchange_weak(
    atomic<_Tp>* __o, typename atomic<_Tp>::value_type* __e, typename atomic<_Tp>::value_type __d) _NOEXCEPT {
  return __o->compare_exchange_weak(*__e, __d);
}

// atomic_compare_exchange_strong

template <class _Tp>
_LIBCPP_HIDE_FROM_ABI bool atomic_compare_exchange_strong(
    volatile atomic<_Tp>* __o, typename atomic<_Tp>::value_type* __e, typename atomic<_Tp>::value_type __d) _NOEXCEPT {
  return __o->compare_exchange_strong(*__e, __d);
}

template <class _Tp>
_LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 bool atomic_compare_exchange_strong(
    atomic<_Tp>* __o, typename atomic<_Tp>::value_type* __e, typename atomic<_Tp>::value_type __d) _NOEXCEPT {
  return __o->compare_exchange_strong(*__e, __d);
}

// atomic_compare_exchange_weak_explicit

template <class _Tp>
_LIBCPP_HIDE_FROM_ABI bool atomic_compare_exchange_weak_explicit(
    volatile atomic<_Tp>* __o,
    typename atomic<_Tp>::value_type* __e,
    typename atomic<_Tp>::value_type __d,
    memory_order __s,
    memory_order __f) _NOEXCEPT _LIBCPP_CHECK_EXCHANGE_MEMORY_ORDER(__s, __f) {
  return __o->compare_exchange_weak(*__e, __d, __s, __f);
}

template <class _Tp>
_LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 bool atomic_compare_exchange_weak_explicit(
    atomic<_Tp>* __o,
    typename atomic<_Tp>::value_type* __e,
    typename atomic<_Tp>::value_type __d,
    memory_order __s,
    memory_order __f) _NOEXCEPT _LIBCPP_CHECK_EXCHANGE_MEMORY_ORDER(__s, __f) {
  return __o->compare_exchange_weak(*__e, __d, __s, __f);
}

// atomic_compare_exchange_strong_explicit

template <class _Tp>
_LIBCPP_HIDE_FROM_ABI bool atomic_compare_exchange_strong_explicit(
    volatile atomic<_Tp>* __o,
    typename atomic<_Tp>::value_type* __e,
    typename atomic<_Tp>::value_type __d,
    memory_order __s,
    memory_order __f) _NOEXCEPT _LIBCPP_CHECK_EXCHANGE_MEMORY_ORDER(__s, __f) {
  return __o->compare_exchange_strong(*__e, __d, __s, __f);
}

template <class _Tp>
_LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 bool atomic_compare_exchange_strong_explicit(
    atomic<_Tp>* __o,
    typename atomic<_Tp>::value_type* __e,
    typename atomic<_Tp>::value_type __d,
    memory_order __s,
    memory_order __f) _NOEXCEPT _LIBCPP_CHECK_EXCHANGE_MEMORY_ORDER(__s, __f) {
  return __o->compare_exchange_strong(*__e, __d, __s, __f);
}

#if _LIBCPP_STD_VER >= 20

// atomic_wait

template <class _Tp>
_LIBCPP_HIDE_FROM_ABI void
atomic_wait(const volatile atomic<_Tp>* __o, typename atomic<_Tp>::value_type __v) _NOEXCEPT {
  return __o->wait(__v);
}

template <class _Tp>
<<<<<<< HEAD
_LIBCPP_AVAILABILITY_SYNC _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 void
atomic_wait(const atomic<_Tp>* __o, typename atomic<_Tp>::value_type __v) _NOEXCEPT {
=======
_LIBCPP_HIDE_FROM_ABI void atomic_wait(const atomic<_Tp>* __o, typename atomic<_Tp>::value_type __v) _NOEXCEPT {
>>>>>>> refs/tags/llvmorg-22.1.8
  return __o->wait(__v);
}

// atomic_wait_explicit

template <class _Tp>
_LIBCPP_HIDE_FROM_ABI void
atomic_wait_explicit(const volatile atomic<_Tp>* __o, typename atomic<_Tp>::value_type __v, memory_order __m) _NOEXCEPT
    _LIBCPP_CHECK_LOAD_MEMORY_ORDER(__m) {
  return __o->wait(__v, __m);
}

template <class _Tp>
<<<<<<< HEAD
_LIBCPP_AVAILABILITY_SYNC _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 void
=======
_LIBCPP_HIDE_FROM_ABI void
>>>>>>> refs/tags/llvmorg-22.1.8
atomic_wait_explicit(const atomic<_Tp>* __o, typename atomic<_Tp>::value_type __v, memory_order __m) _NOEXCEPT
    _LIBCPP_CHECK_LOAD_MEMORY_ORDER(__m) {
  return __o->wait(__v, __m);
}

// atomic_notify_one

template <class _Tp>
_LIBCPP_HIDE_FROM_ABI void atomic_notify_one(volatile atomic<_Tp>* __o) _NOEXCEPT {
  __o->notify_one();
}
template <class _Tp>
<<<<<<< HEAD
_LIBCPP_AVAILABILITY_SYNC _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 void
atomic_notify_one(atomic<_Tp>* __o) _NOEXCEPT {
=======
_LIBCPP_HIDE_FROM_ABI void atomic_notify_one(atomic<_Tp>* __o) _NOEXCEPT {
>>>>>>> refs/tags/llvmorg-22.1.8
  __o->notify_one();
}

// atomic_notify_all

template <class _Tp>
_LIBCPP_HIDE_FROM_ABI void atomic_notify_all(volatile atomic<_Tp>* __o) _NOEXCEPT {
  __o->notify_all();
}
template <class _Tp>
<<<<<<< HEAD
_LIBCPP_AVAILABILITY_SYNC _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 void
atomic_notify_all(atomic<_Tp>* __o) _NOEXCEPT {
=======
_LIBCPP_HIDE_FROM_ABI void atomic_notify_all(atomic<_Tp>* __o) _NOEXCEPT {
>>>>>>> refs/tags/llvmorg-22.1.8
  __o->notify_all();
}

#endif // _LIBCPP_STD_VER >= 20

// atomic_fetch_add

template <class _Tp>
_LIBCPP_HIDE_FROM_ABI _Tp
atomic_fetch_add(volatile atomic<_Tp>* __o, typename atomic<_Tp>::difference_type __op) _NOEXCEPT {
  return __o->fetch_add(__op);
}

template <class _Tp>
_LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _Tp
atomic_fetch_add(atomic<_Tp>* __o, typename atomic<_Tp>::difference_type __op) _NOEXCEPT {
  return __o->fetch_add(__op);
}

// atomic_fetch_add_explicit

template <class _Tp>
_LIBCPP_HIDE_FROM_ABI _Tp atomic_fetch_add_explicit(
    volatile atomic<_Tp>* __o, typename atomic<_Tp>::difference_type __op, memory_order __m) _NOEXCEPT {
  return __o->fetch_add(__op, __m);
}

template <class _Tp>
_LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _Tp
atomic_fetch_add_explicit(atomic<_Tp>* __o, typename atomic<_Tp>::difference_type __op, memory_order __m) _NOEXCEPT {
  return __o->fetch_add(__op, __m);
}

// atomic_fetch_sub

template <class _Tp>
_LIBCPP_HIDE_FROM_ABI _Tp
atomic_fetch_sub(volatile atomic<_Tp>* __o, typename atomic<_Tp>::difference_type __op) _NOEXCEPT {
  return __o->fetch_sub(__op);
}

template <class _Tp>
_LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _Tp
atomic_fetch_sub(atomic<_Tp>* __o, typename atomic<_Tp>::difference_type __op) _NOEXCEPT {
  return __o->fetch_sub(__op);
}

// atomic_fetch_sub_explicit

template <class _Tp>
_LIBCPP_HIDE_FROM_ABI _Tp atomic_fetch_sub_explicit(
    volatile atomic<_Tp>* __o, typename atomic<_Tp>::difference_type __op, memory_order __m) _NOEXCEPT {
  return __o->fetch_sub(__op, __m);
}

template <class _Tp>
_LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _Tp
atomic_fetch_sub_explicit(atomic<_Tp>* __o, typename atomic<_Tp>::difference_type __op, memory_order __m) _NOEXCEPT {
  return __o->fetch_sub(__op, __m);
}

// atomic_fetch_and

template <class _Tp, __enable_if_t<is_integral<_Tp>::value && !is_same<_Tp, bool>::value, int> = 0>
_LIBCPP_HIDE_FROM_ABI _Tp atomic_fetch_and(volatile atomic<_Tp>* __o, typename atomic<_Tp>::value_type __op) _NOEXCEPT {
  return __o->fetch_and(__op);
}

template <class _Tp, __enable_if_t<is_integral<_Tp>::value && !is_same<_Tp, bool>::value, int> = 0>
_LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _Tp
atomic_fetch_and(atomic<_Tp>* __o, typename atomic<_Tp>::value_type __op) _NOEXCEPT {
  return __o->fetch_and(__op);
}

// atomic_fetch_and_explicit

template <class _Tp, __enable_if_t<is_integral<_Tp>::value && !is_same<_Tp, bool>::value, int> = 0>
_LIBCPP_HIDE_FROM_ABI _Tp atomic_fetch_and_explicit(
    volatile atomic<_Tp>* __o, typename atomic<_Tp>::value_type __op, memory_order __m) _NOEXCEPT {
  return __o->fetch_and(__op, __m);
}

template <class _Tp, __enable_if_t<is_integral<_Tp>::value && !is_same<_Tp, bool>::value, int> = 0>
_LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _Tp
atomic_fetch_and_explicit(atomic<_Tp>* __o, typename atomic<_Tp>::value_type __op, memory_order __m) _NOEXCEPT {
  return __o->fetch_and(__op, __m);
}

// atomic_fetch_or

template <class _Tp, __enable_if_t<is_integral<_Tp>::value && !is_same<_Tp, bool>::value, int> = 0>
_LIBCPP_HIDE_FROM_ABI _Tp atomic_fetch_or(volatile atomic<_Tp>* __o, typename atomic<_Tp>::value_type __op) _NOEXCEPT {
  return __o->fetch_or(__op);
}

template <class _Tp, __enable_if_t<is_integral<_Tp>::value && !is_same<_Tp, bool>::value, int> = 0>
_LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _Tp
atomic_fetch_or(atomic<_Tp>* __o, typename atomic<_Tp>::value_type __op) _NOEXCEPT {
  return __o->fetch_or(__op);
}

// atomic_fetch_or_explicit

template <class _Tp, __enable_if_t<is_integral<_Tp>::value && !is_same<_Tp, bool>::value, int> = 0>
_LIBCPP_HIDE_FROM_ABI _Tp
atomic_fetch_or_explicit(volatile atomic<_Tp>* __o, typename atomic<_Tp>::value_type __op, memory_order __m) _NOEXCEPT {
  return __o->fetch_or(__op, __m);
}

template <class _Tp, __enable_if_t<is_integral<_Tp>::value && !is_same<_Tp, bool>::value, int> = 0>
_LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _Tp
atomic_fetch_or_explicit(atomic<_Tp>* __o, typename atomic<_Tp>::value_type __op, memory_order __m) _NOEXCEPT {
  return __o->fetch_or(__op, __m);
}

// atomic_fetch_xor

template <class _Tp, __enable_if_t<is_integral<_Tp>::value && !is_same<_Tp, bool>::value, int> = 0>
_LIBCPP_HIDE_FROM_ABI _Tp atomic_fetch_xor(volatile atomic<_Tp>* __o, typename atomic<_Tp>::value_type __op) _NOEXCEPT {
  return __o->fetch_xor(__op);
}

template <class _Tp, __enable_if_t<is_integral<_Tp>::value && !is_same<_Tp, bool>::value, int> = 0>
_LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _Tp
atomic_fetch_xor(atomic<_Tp>* __o, typename atomic<_Tp>::value_type __op) _NOEXCEPT {
  return __o->fetch_xor(__op);
}

// atomic_fetch_xor_explicit

template <class _Tp, __enable_if_t<is_integral<_Tp>::value && !is_same<_Tp, bool>::value, int> = 0>
_LIBCPP_HIDE_FROM_ABI _Tp atomic_fetch_xor_explicit(
    volatile atomic<_Tp>* __o, typename atomic<_Tp>::value_type __op, memory_order __m) _NOEXCEPT {
  return __o->fetch_xor(__op, __m);
}

template <class _Tp, __enable_if_t<is_integral<_Tp>::value && !is_same<_Tp, bool>::value, int> = 0>
_LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _Tp
atomic_fetch_xor_explicit(atomic<_Tp>* __o, typename atomic<_Tp>::value_type __op, memory_order __m) _NOEXCEPT {
  return __o->fetch_xor(__op, __m);
}

#if _LIBCPP_STD_VER >= 26

// atomic_fetch_max

template <class _Tp>
_LIBCPP_HIDE_FROM_ABI _Tp atomic_fetch_max(volatile atomic<_Tp>* __o, typename atomic<_Tp>::value_type __op) _NOEXCEPT {
  return __o->fetch_max(__op);
}

template <class _Tp>
_LIBCPP_HIDE_FROM_ABI constexpr _Tp
atomic_fetch_max(atomic<_Tp>* __o, typename atomic<_Tp>::value_type __op) _NOEXCEPT {
  return __o->fetch_max(__op);
}

// atomic_fetch_max_explicit

template <class _Tp>
_LIBCPP_HIDE_FROM_ABI _Tp atomic_fetch_max_explicit(
    volatile atomic<_Tp>* __o, typename atomic<_Tp>::value_type __op, memory_order __m) _NOEXCEPT {
  return __o->fetch_max(__op, __m);
}

template <class _Tp>
_LIBCPP_HIDE_FROM_ABI constexpr _Tp
atomic_fetch_max_explicit(atomic<_Tp>* __o, typename atomic<_Tp>::value_type __op, memory_order __m) _NOEXCEPT {
  return __o->fetch_max(__op, __m);
}

// atomic_fetch_min

template <class _Tp>
_LIBCPP_HIDE_FROM_ABI _Tp atomic_fetch_min(volatile atomic<_Tp>* __o, typename atomic<_Tp>::value_type __op) _NOEXCEPT {
  return __o->fetch_min(__op);
}

template <class _Tp>
_LIBCPP_HIDE_FROM_ABI constexpr _Tp
atomic_fetch_min(atomic<_Tp>* __o, typename atomic<_Tp>::value_type __op) _NOEXCEPT {
  return __o->fetch_min(__op);
}

// atomic_fetch_min_explicit

template <class _Tp>
_LIBCPP_HIDE_FROM_ABI _Tp atomic_fetch_min_explicit(
    volatile atomic<_Tp>* __o, typename atomic<_Tp>::value_type __op, memory_order __m) _NOEXCEPT {
  return __o->fetch_min(__op, __m);
}

template <class _Tp>
_LIBCPP_HIDE_FROM_ABI constexpr _Tp
atomic_fetch_min_explicit(atomic<_Tp>* __o, typename atomic<_Tp>::value_type __op, memory_order __m) _NOEXCEPT {
  return __o->fetch_min(__op, __m);
}

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___ATOMIC_ATOMIC_H
