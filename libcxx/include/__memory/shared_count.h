//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___MEMORY_SHARED_COUNT_H
#define _LIBCPP___MEMORY_SHARED_COUNT_H

#include <__config>
#include <__memory/addressof.h>
#include <typeinfo>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

// NOTE: Relaxed and acq/rel atomics (for increment and decrement respectively)
// should be sufficient for thread safety.
// See https://llvm.org/PR22803

template <class _Tp>
inline _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _Tp __libcpp_atomic_refcount_increment(_Tp& __t) _NOEXCEPT {
#if _LIBCPP_STD_VER >= 26
  if consteval {
    return ++__t;
  }
#endif
#if _LIBCPP_HAS_THREADS
  return __atomic_add_fetch(std::addressof(__t), 1, __ATOMIC_RELAXED);
#else
  return __t += 1;
#endif
}

template <class _Tp>
inline _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 _Tp __libcpp_atomic_refcount_decrement(_Tp& __t) _NOEXCEPT {
#if _LIBCPP_STD_VER >= 26
  if consteval {
    return --__t;
  }
#endif
#if _LIBCPP_HAS_THREADS
  return __atomic_add_fetch(std::addressof(__t), -1, __ATOMIC_ACQ_REL);
#else
  return __t -= 1;
#endif
}

class _LIBCPP_EXPORTED_FROM_ABI __shared_count {
  __shared_count(const __shared_count&);
  __shared_count& operator=(const __shared_count&);

protected:
  long __shared_owners_;
  _LIBCPP_CONSTEXPR_SINCE_CXX26 virtual ~__shared_count() {}

private:
  _LIBCPP_CONSTEXPR_SINCE_CXX26 virtual void __on_zero_shared() _NOEXCEPT = 0;

public:
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 explicit __shared_count(long __refs = 0) _NOEXCEPT
      : __shared_owners_(__refs) {}

#if defined(_LIBCPP_SHARED_PTR_DEFINE_LEGACY_INLINE_FUNCTIONS)
  void __add_shared() noexcept;
  bool __release_shared() noexcept;
#else
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 void __add_shared() _NOEXCEPT {
    __libcpp_atomic_refcount_increment(__shared_owners_);
  }
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 bool __release_shared() _NOEXCEPT {
    if (__libcpp_atomic_refcount_decrement(__shared_owners_) == -1) {
      __on_zero_shared();
      return true;
    }
    return false;
  }
#endif
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 long use_count() const _NOEXCEPT {
#if _LIBCPP_STD_VER >= 26
    if consteval {
      return __shared_owners_ + 1;
    }
#endif
#if _LIBCPP_HAS_THREADS
    return __atomic_load_n(&__shared_owners_, __ATOMIC_RELAXED) + 1;
#else
    return __shared_owners_ + 1;
#endif
  }
};

class _LIBCPP_EXPORTED_FROM_ABI __shared_weak_count : private __shared_count {
  long __shared_weak_owners_;

public:
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 explicit __shared_weak_count(long __refs = 0) _NOEXCEPT
      : __shared_count(__refs),
        __shared_weak_owners_(__refs) {}

protected:
  _LIBCPP_CONSTEXPR_SINCE_CXX26 ~__shared_weak_count() override {}

public:
#if defined(_LIBCPP_SHARED_PTR_DEFINE_LEGACY_INLINE_FUNCTIONS)
  void __add_shared() noexcept;
  void __add_weak() noexcept;
  void __release_shared() noexcept;
#else
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 void __add_shared() _NOEXCEPT { __shared_count::__add_shared(); }
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 void __add_weak() _NOEXCEPT {
    __libcpp_atomic_refcount_increment(__shared_weak_owners_);
  }
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 void __release_shared() _NOEXCEPT {
    if (__shared_count::__release_shared())
      __release_weak_constexpr();
  }
#endif
  void __release_weak() _NOEXCEPT;
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 void __release_weak_constexpr() _NOEXCEPT {
#if _LIBCPP_STD_VER >= 26
    if consteval {
      if (__shared_weak_owners_ == 0)
        __on_zero_shared_weak();
      else if (--__shared_weak_owners_ == -1)
        __on_zero_shared_weak();
      return;
    }
#endif
    __release_weak();
  }
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 long use_count() const _NOEXCEPT {
    return __shared_count::use_count();
  }
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_SINCE_CXX26 __shared_weak_count* __lock_constexpr() _NOEXCEPT {
#if _LIBCPP_STD_VER >= 26
    if consteval {
      if (__shared_owners_ == -1)
        return nullptr;
      ++__shared_owners_;
      return this;
    }
#endif
    return lock();
  }
  __shared_weak_count* lock() _NOEXCEPT;

  _LIBCPP_CONSTEXPR_SINCE_CXX26 virtual const void* __get_deleter(const type_info&) const _NOEXCEPT;

private:
  _LIBCPP_CONSTEXPR_SINCE_CXX26 virtual void __on_zero_shared_weak() _NOEXCEPT = 0;
};

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___MEMORY_SHARED_COUNT_H
