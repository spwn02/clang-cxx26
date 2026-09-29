//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___EXCEPTION_EXCEPTION_PTR_H
#define _LIBCPP___EXCEPTION_EXCEPTION_PTR_H

#include <__config>
#include <__cstddef/nullptr_t.h>
#include <__cstddef/size_t.h>
#include <__exception/operations.h>
#include <__memory/addressof.h>
#include <__memory/construct_at.h>
#include <__type_traits/decay.h>
#include <__type_traits/is_array.h>
#include <__type_traits/is_member_pointer.h>
#include <__type_traits/is_pointer.h>
#include <__type_traits/is_same.h>
#include <__type_traits/remove_cv.h>
#include <__utility/move.h>
#include <__utility/swap.h>
#include <__verbose_abort>
#include <typeinfo>

#if _LIBCPP_STD_VER >= 26
#  include <optional>
#endif

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_PUSH_MACROS
#include <__undef_macros>

#ifndef _LIBCPP_ABI_MICROSOFT

#  if _LIBCPP_AVAILABILITY_HAS_INIT_PRIMARY_EXCEPTION

namespace __cxxabiv1 {

extern "C" {
_LIBCPP_OVERRIDABLE_FUNC_VIS void* __cxa_allocate_exception(std::size_t) throw();
_LIBCPP_OVERRIDABLE_FUNC_VIS void __cxa_free_exception(void*) throw();

struct __cxa_exception;
_LIBCPP_OVERRIDABLE_FUNC_VIS __cxa_exception* __cxa_init_primary_exception(
    void*,
    std::type_info*,
#    if defined(_WIN32)
    void(__thiscall*)(void*)) throw();
#    elif defined(__wasm__)
    // In Wasm, a destructor returns its argument
    void* (*)(void*)) throw();
#    else
    void (*)(void*)) throw();
#    endif
}

} // namespace __cxxabiv1

#  endif

#endif

_LIBCPP_BEGIN_UNVERSIONED_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26 && !defined(_LIBCPP_BUILDING_LIBRARY) && \
    !defined(_LIBCPP_HAS_NO_EXCEPTIONS) && \
    __has_builtin(__builtin_constexpr_exception_capture) && \
    __has_builtin(__builtin_constexpr_exception_retain) && \
    __has_builtin(__builtin_constexpr_exception_release) && \
    __has_builtin(__builtin_constexpr_exception_rethrow)
#  define _LIBCPP_HAS_CONSTEXPR_EXCEPTION_PTR 1
#  define _LIBCPP_CONSTEXPR_EXCEPTION_PTR constexpr
// Constant-evaluation hooks map handles to stable evaluator-owned exception
// objects. The Consteval builtins diagnose accidental runtime use.
_LIBCPP_EXPORTED_FROM_ABI void __exception_ptr_copy_runtime(exception_ptr*, const exception_ptr&) noexcept;
_LIBCPP_EXPORTED_FROM_ABI void __exception_ptr_assign_runtime(exception_ptr*, const exception_ptr&) noexcept;
_LIBCPP_EXPORTED_FROM_ABI void __exception_ptr_destroy_runtime(exception_ptr*) noexcept;
_LIBCPP_EXPORTED_FROM_ABI void __exception_ptr_default_runtime(exception_ptr*) noexcept;
_LIBCPP_EXPORTED_FROM_ABI void __exception_ptr_assign_null_runtime(exception_ptr*) noexcept;
_LIBCPP_EXPORTED_FROM_ABI bool __exception_ptr_to_bool_runtime(const exception_ptr*) noexcept;
_LIBCPP_EXPORTED_FROM_ABI bool __exception_ptr_equal_runtime(const exception_ptr*, const exception_ptr*) noexcept;
_LIBCPP_EXPORTED_FROM_ABI void __exception_ptr_swap_runtime(exception_ptr*, exception_ptr*) noexcept;
_LIBCPP_EXPORTED_FROM_ABI exception_ptr __current_exception_runtime() noexcept;
[[noreturn]] _LIBCPP_EXPORTED_FROM_ABI void __rethrow_exception_runtime(exception_ptr);
#endif
#ifndef _LIBCPP_CONSTEXPR_EXCEPTION_PTR
#  define _LIBCPP_CONSTEXPR_EXCEPTION_PTR
#endif

#ifndef _LIBCPP_ABI_MICROSOFT

inline _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_EXCEPTION_PTR void
swap(exception_ptr& __x, exception_ptr& __y) _NOEXCEPT;

class _LIBCPP_EXPORTED_FROM_ABI exception_ptr {
  void* __ptr_;

  static exception_ptr __from_native_exception_pointer(void*) _NOEXCEPT;

  template <class _Ep>
  friend _LIBCPP_HIDE_FROM_ABI exception_ptr __make_exception_ptr_explicit(_Ep&) _NOEXCEPT;

public:
  // exception_ptr is basically a COW string so it is trivially relocatable.
  using __trivially_relocatable _LIBCPP_NODEBUG = exception_ptr;

  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_EXCEPTION_PTR exception_ptr() _NOEXCEPT : __ptr_() {}
  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_EXCEPTION_PTR exception_ptr(nullptr_t) _NOEXCEPT : __ptr_() {}

#  if defined(_LIBCPP_HAS_CONSTEXPR_EXCEPTION_PTR)
  _LIBCPP_HIDE_FROM_ABI constexpr exception_ptr(const exception_ptr& __other) _NOEXCEPT
      : __ptr_(nullptr) {
    if consteval {
      __ptr_ = __other.__ptr_;
      if (__ptr_)
        __builtin_constexpr_exception_retain(__ptr_);
    } else {
      __exception_ptr_copy_runtime(this, __other);
    }
  }
  _LIBCPP_HIDE_FROM_ABI constexpr exception_ptr(exception_ptr&& __other) _NOEXCEPT
      : __ptr_(__other.__ptr_) {
    __other.__ptr_ = nullptr;
  }
  _LIBCPP_HIDE_FROM_ABI constexpr exception_ptr& operator=(const exception_ptr& __other) _NOEXCEPT {
    if (this != &__other) {
      if consteval {
        if (__ptr_)
          __builtin_constexpr_exception_release(__ptr_);
        __ptr_ = __other.__ptr_;
        if (__ptr_)
          __builtin_constexpr_exception_retain(__ptr_);
      } else {
        __exception_ptr_assign_runtime(this, __other);
      }
    }
    return *this;
  }
  _LIBCPP_HIDE_FROM_ABI constexpr exception_ptr& operator=(exception_ptr&& __other) _NOEXCEPT {
    exception_ptr __tmp(std::move(__other));
    std::swap(__tmp, *this);
    return *this;
  }
  _LIBCPP_HIDE_FROM_ABI constexpr ~exception_ptr() _NOEXCEPT {
    if consteval {
      if (__ptr_)
        __builtin_constexpr_exception_release(__ptr_);
    } else {
      __exception_ptr_destroy_runtime(this);
    }
  }
#  else
  exception_ptr(const exception_ptr&) _NOEXCEPT;
  _LIBCPP_HIDE_FROM_ABI exception_ptr(exception_ptr&& __other) _NOEXCEPT : __ptr_(__other.__ptr_) {
    __other.__ptr_ = nullptr;
  }
  exception_ptr& operator=(const exception_ptr&) _NOEXCEPT;
  _LIBCPP_HIDE_FROM_ABI exception_ptr& operator=(exception_ptr&& __other) _NOEXCEPT {
    exception_ptr __tmp(std::move(__other));
    std::swap(__tmp, *this);
    return *this;
  }
  ~exception_ptr() _NOEXCEPT;
#  endif

  _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_EXCEPTION_PTR explicit operator bool() const _NOEXCEPT { return __ptr_ != nullptr; }

  friend _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_EXCEPTION_PTR bool operator==(const exception_ptr& __x, const exception_ptr& __y) _NOEXCEPT;

  friend _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_EXCEPTION_PTR bool operator!=(const exception_ptr& __x, const exception_ptr& __y) _NOEXCEPT {
    return !(__x == __y);
  }

  friend _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_EXCEPTION_PTR void
  swap(exception_ptr& __x, exception_ptr& __y) _NOEXCEPT;

#  if defined(_LIBCPP_HAS_CONSTEXPR_EXCEPTION_PTR)
  friend _LIBCPP_HIDE_FROM_ABI constexpr exception_ptr current_exception() _NOEXCEPT;
  friend __attribute__((noreturn)) _LIBCPP_HIDE_FROM_ABI constexpr void
  rethrow_exception(exception_ptr);
#  else
  friend _LIBCPP_EXPORTED_FROM_ABI exception_ptr current_exception() _NOEXCEPT;
  friend _LIBCPP_EXPORTED_FROM_ABI void rethrow_exception(exception_ptr);
#  endif
};

#  if defined(_LIBCPP_HAS_CONSTEXPR_EXCEPTION_PTR)
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr exception_ptr current_exception() _NOEXCEPT;
__attribute__((noreturn)) _LIBCPP_HIDE_FROM_ABI constexpr void
rethrow_exception(exception_ptr);
#  endif

inline _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_EXCEPTION_PTR void
swap(exception_ptr& __x, exception_ptr& __y) _NOEXCEPT {
#  if defined(_LIBCPP_HAS_CONSTEXPR_EXCEPTION_PTR)
  if consteval {
    std::swap(__x.__ptr_, __y.__ptr_);
  } else {
    __exception_ptr_swap_runtime(&__x, &__y);
  }
#  else
  std::swap(__x.__ptr_, __y.__ptr_);
#  endif
}

#  if defined(_LIBCPP_HAS_CONSTEXPR_EXCEPTION_PTR)
inline _LIBCPP_HIDE_FROM_ABI constexpr bool
operator==(const exception_ptr& __x, const exception_ptr& __y) _NOEXCEPT {
  if consteval {
    return __x.__ptr_ == __y.__ptr_;
  } else {
    return __exception_ptr_equal_runtime(&__x, &__y);
  }
}
#  else
inline _LIBCPP_HIDE_FROM_ABI bool
operator==(const exception_ptr& __x, const exception_ptr& __y) _NOEXCEPT {
  return __x.__ptr_ == __y.__ptr_;
}
#  endif

#  if _LIBCPP_HAS_EXCEPTIONS
#    if _LIBCPP_AVAILABILITY_HAS_INIT_PRIMARY_EXCEPTION
template <class _Ep>
_LIBCPP_HIDE_FROM_ABI exception_ptr __make_exception_ptr_explicit(_Ep& __e) _NOEXCEPT {
  using _Ep2 = __decay_t<_Ep>;
  void* __ex = __cxxabiv1::__cxa_allocate_exception(sizeof(_Ep));
#      ifdef __wasm__
  auto __cleanup = [](void* __p) -> void* {
    std::__destroy_at(static_cast<_Ep2*>(__p));
    return __p;
  };
#      else
  auto __cleanup = [](void* __p) { std::__destroy_at(static_cast<_Ep2*>(__p)); };
#      endif
  (void)__cxxabiv1::__cxa_init_primary_exception(__ex, const_cast<std::type_info*>(&typeid(_Ep)), __cleanup);

  try {
    ::new (__ex) _Ep2(__e);
    return exception_ptr::__from_native_exception_pointer(__ex);
  } catch (...) {
    __cxxabiv1::__cxa_free_exception(__ex);
    return current_exception();
  }
}
#    endif

template <class _Ep>
_LIBCPP_HIDE_FROM_ABI exception_ptr __make_exception_ptr_via_throw(_Ep& __e) _NOEXCEPT {
  try {
    throw __e;
  } catch (...) {
    return current_exception();
  }
}

template <class _Ep>
_LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_EXCEPTION_PTR exception_ptr make_exception_ptr(_Ep __e) _NOEXCEPT {
#if defined(_LIBCPP_HAS_CONSTEXPR_EXCEPTION_PTR)
  if consteval {
    try {
      throw __e;
    } catch (...) {
      return current_exception();
    }
  }
#endif
  // Objective-C exceptions are thrown via pointer. When throwing an Objective-C exception,
  // Clang generates a call to `objc_exception_throw` instead of the usual `__cxa_throw`.
  // That function creates an exception with a special Objective-C typeinfo instead of
  // the usual C++ typeinfo, since that is needed to implement the behavior documented
  // at [1]).
  //
  // Because of this special behavior, we can't create an exception via `__cxa_init_primary_exception`
  // for Objective-C exceptions, otherwise we'd bypass `objc_exception_throw`. See https://llvm.org/PR135089.
  //
  // [1]:
  // https://developer.apple.com/library/archive/documentation/Cocoa/Conceptual/Exceptions/Articles/Exceptions64Bit.html
  if _LIBCPP_CONSTEXPR (is_pointer<_Ep>::value) {
    return std::__make_exception_ptr_via_throw(__e);
  }

#    if _LIBCPP_AVAILABILITY_HAS_INIT_PRIMARY_EXCEPTION && !defined(_LIBCPP_CXX03_LANG)
  return std::__make_exception_ptr_explicit(__e);
#    else
  return std::__make_exception_ptr_via_throw(__e);
#    endif
}
#  else  // !_LIBCPP_HAS_EXCEPTIONS
template <class _Ep>
_LIBCPP_HIDE_FROM_ABI exception_ptr make_exception_ptr(_Ep) _NOEXCEPT {
  _LIBCPP_VERBOSE_ABORT("make_exception_ptr was called in -fno-exceptions mode");
}
#  endif // _LIBCPP_HAS_EXCEPTIONS

#else // _LIBCPP_ABI_MICROSOFT

class _LIBCPP_EXPORTED_FROM_ABI exception_ptr {
  _LIBCPP_DIAGNOSTIC_PUSH
  _LIBCPP_CLANG_DIAGNOSTIC_IGNORED("-Wunused-private-field")
  void* __ptr1_;
  void* __ptr2_;
  _LIBCPP_DIAGNOSTIC_POP

public:
#  if defined(_LIBCPP_HAS_CONSTEXPR_EXCEPTION_PTR)
  _LIBCPP_HIDE_FROM_ABI constexpr exception_ptr() _NOEXCEPT
      : __ptr1_(nullptr), __ptr2_(nullptr) {
    if !consteval {
      __exception_ptr_default_runtime(this);
    }
  }
  _LIBCPP_HIDE_FROM_ABI constexpr exception_ptr(nullptr_t) _NOEXCEPT
      : __ptr1_(nullptr), __ptr2_(nullptr) {
    if !consteval {
      __exception_ptr_default_runtime(this);
    }
  }
  _LIBCPP_HIDE_FROM_ABI constexpr exception_ptr(const exception_ptr& __other) _NOEXCEPT
      : __ptr1_(nullptr), __ptr2_(nullptr) {
    if consteval {
      __ptr1_ = __other.__ptr1_;
      if (__ptr1_)
        __builtin_constexpr_exception_retain(__ptr1_);
    } else {
      __exception_ptr_copy_runtime(this, __other);
    }
  }
  _LIBCPP_HIDE_FROM_ABI constexpr exception_ptr(exception_ptr&& __other) _NOEXCEPT
      : __ptr1_(nullptr), __ptr2_(nullptr) {
    if consteval {
      __ptr1_ = __other.__ptr1_;
      __ptr2_ = __other.__ptr2_;
      __other.__ptr1_ = __other.__ptr2_ = nullptr;
    } else {
      __exception_ptr_default_runtime(this);
      __exception_ptr_swap_runtime(this, &__other);
    }
  }
  _LIBCPP_HIDE_FROM_ABI constexpr exception_ptr& operator=(const exception_ptr& __other) _NOEXCEPT {
    if (this != &__other) {
      if consteval {
        if (__ptr1_)
          __builtin_constexpr_exception_release(__ptr1_);
        __ptr1_ = __other.__ptr1_;
        __ptr2_ = nullptr;
        if (__ptr1_)
          __builtin_constexpr_exception_retain(__ptr1_);
      } else {
        __exception_ptr_assign_runtime(this, __other);
      }
    }
    return *this;
  }
  _LIBCPP_HIDE_FROM_ABI constexpr exception_ptr& operator=(exception_ptr&& __other) _NOEXCEPT {
    if (this != &__other) {
      if consteval {
        if (__ptr1_)
          __builtin_constexpr_exception_release(__ptr1_);
        __ptr1_ = __other.__ptr1_;
        __ptr2_ = __other.__ptr2_;
        __other.__ptr1_ = __other.__ptr2_ = nullptr;
      } else {
        __exception_ptr_swap_runtime(this, &__other);
        __exception_ptr_assign_null_runtime(&__other);
      }
    }
    return *this;
  }
  _LIBCPP_HIDE_FROM_ABI constexpr exception_ptr& operator=(nullptr_t) _NOEXCEPT {
    if consteval {
      if (__ptr1_)
        __builtin_constexpr_exception_release(__ptr1_);
      __ptr1_ = nullptr;
      __ptr2_ = nullptr;
    } else {
      __exception_ptr_assign_null_runtime(this);
    }
    return *this;
  }
  _LIBCPP_HIDE_FROM_ABI constexpr ~exception_ptr() _NOEXCEPT {
    if consteval {
      if (__ptr1_)
        __builtin_constexpr_exception_release(__ptr1_);
    } else {
      __exception_ptr_destroy_runtime(this);
    }
  }
  _LIBCPP_HIDE_FROM_ABI constexpr explicit operator bool() const _NOEXCEPT {
    if consteval {
      return __ptr1_ != nullptr;
    } else {
      return __exception_ptr_to_bool_runtime(this);
    }
  }
  friend _LIBCPP_HIDE_FROM_ABI constexpr bool operator==(const exception_ptr&, const exception_ptr&) _NOEXCEPT;
  friend _LIBCPP_HIDE_FROM_ABI constexpr void swap(exception_ptr&, exception_ptr&) _NOEXCEPT;
  friend _LIBCPP_HIDE_FROM_ABI constexpr exception_ptr current_exception() _NOEXCEPT;
  friend _LIBCPP_HIDE_FROM_ABI constexpr void rethrow_exception(exception_ptr)
      __attribute__((noreturn));
#  else
  exception_ptr() _NOEXCEPT;
  exception_ptr(nullptr_t) _NOEXCEPT;
  exception_ptr(const exception_ptr& __other) _NOEXCEPT;
  exception_ptr& operator=(const exception_ptr& __other) _NOEXCEPT;
  exception_ptr& operator=(nullptr_t) _NOEXCEPT;
  ~exception_ptr() _NOEXCEPT;
  explicit operator bool() const _NOEXCEPT;
#  endif
};

#  if defined(_LIBCPP_HAS_CONSTEXPR_EXCEPTION_PTR)
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr exception_ptr current_exception() _NOEXCEPT;
_LIBCPP_HIDE_FROM_ABI constexpr void rethrow_exception(exception_ptr)
    __attribute__((noreturn));
#  endif

#  if defined(_LIBCPP_HAS_CONSTEXPR_EXCEPTION_PTR)
_LIBCPP_HIDE_FROM_ABI constexpr bool operator==(const exception_ptr& __x, const exception_ptr& __y) _NOEXCEPT {
  if consteval {
    return __x.__ptr1_ == __y.__ptr1_;
  } else {
    return __exception_ptr_equal_runtime(&__x, &__y);
  }
}
#  else
_LIBCPP_EXPORTED_FROM_ABI bool operator==(const exception_ptr& __x, const exception_ptr& __y) _NOEXCEPT;
#  endif

inline _LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_EXCEPTION_PTR bool operator!=(const exception_ptr& __x, const exception_ptr& __y) _NOEXCEPT {
  return !(__x == __y);
}

#  if defined(_LIBCPP_HAS_CONSTEXPR_EXCEPTION_PTR)
inline _LIBCPP_HIDE_FROM_ABI constexpr void swap(exception_ptr& __x, exception_ptr& __y) _NOEXCEPT {
  if consteval {
    void* __tmp = __x.__ptr1_;
    __x.__ptr1_ = __y.__ptr1_;
    __y.__ptr1_ = __tmp;
    __x.__ptr2_ = __y.__ptr2_ = nullptr;
  } else {
    __exception_ptr_swap_runtime(&__x, &__y);
  }
}
#  else
_LIBCPP_EXPORTED_FROM_ABI void swap(exception_ptr&, exception_ptr&) _NOEXCEPT;
#  endif

_LIBCPP_EXPORTED_FROM_ABI exception_ptr __copy_exception_ptr(void* __except, const void* __ptr);
#  if !defined(_LIBCPP_HAS_CONSTEXPR_EXCEPTION_PTR)
_LIBCPP_EXPORTED_FROM_ABI exception_ptr current_exception() _NOEXCEPT;
[[__noreturn__]] _LIBCPP_EXPORTED_FROM_ABI void rethrow_exception(exception_ptr);
#  endif

// This is a built-in template function which automagically extracts the required
// information.
template <class _E>
void* __GetExceptionInfo(_E);

template <class _Ep>
_LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_EXCEPTION_PTR exception_ptr make_exception_ptr(_Ep __e) _NOEXCEPT {
#if defined(_LIBCPP_HAS_CONSTEXPR_EXCEPTION_PTR)
  if consteval {
    try {
      throw __e;
    } catch (...) {
      return current_exception();
    }
  }
#endif
  return __copy_exception_ptr(std::addressof(__e), __GetExceptionInfo(__e));
}

#endif // _LIBCPP_ABI_MICROSOFT

#if defined(_LIBCPP_HAS_CONSTEXPR_EXCEPTION_PTR)
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr exception_ptr current_exception() _NOEXCEPT {
  if consteval {
    exception_ptr __result;
#  ifndef _LIBCPP_ABI_MICROSOFT
    __result.__ptr_ = __builtin_constexpr_exception_capture();
#  else
    __result.__ptr1_ = __builtin_constexpr_exception_capture();
    __result.__ptr2_ = nullptr;
#  endif
    return __result;
  } else {
    return __current_exception_runtime();
  }
}

__attribute__((noreturn)) _LIBCPP_HIDE_FROM_ABI constexpr void
rethrow_exception(exception_ptr __p) {
  if consteval {
#  ifndef _LIBCPP_ABI_MICROSOFT
    __builtin_constexpr_exception_rethrow(__p.__ptr_);
#  else
    __builtin_constexpr_exception_rethrow(__p.__ptr1_);
#  endif
  } else {
    __rethrow_exception_runtime(std::move(__p));
  }
  __builtin_unreachable();
}
#endif

#if _LIBCPP_STD_VER >= 26
// [propagation], exception_ptr_cast
//
// Portable across both the Itanium and Microsoft exception_ptr representations:
// rethrows into a catch(const _Ep&) to reuse the runtime's own handler-matching
// logic (the same logic `catch` clauses use), rather than reaching into either
// ABI's private exception-object layout.
template <class _Ep>
_LIBCPP_HIDE_FROM_ABI _LIBCPP_CONSTEXPR_EXCEPTION_PTR optional<const _Ep&> exception_ptr_cast(const exception_ptr& __p) _NOEXCEPT {
  static_assert(!is_array<_Ep>::value, "exception_ptr_cast<E>: E must not be an array type");
  static_assert(!is_pointer<_Ep>::value, "exception_ptr_cast<E>: E must not be a pointer type");
  static_assert(!is_member_pointer<_Ep>::value, "exception_ptr_cast<E>: E must not be a pointer-to-member type");
  static_assert(is_same<_Ep, remove_cv_t<_Ep> >::value, "exception_ptr_cast<E>: E must be cv-unqualified");

#  if _LIBCPP_HAS_EXCEPTIONS
  if (!__p)
    return nullopt;
  try {
    std::rethrow_exception(__p);
  } catch (const _Ep& __e) {
    return optional<const _Ep&>(__e);
  } catch (...) {
  }
  return nullopt;
#  else
  (void)__p;
  _LIBCPP_VERBOSE_ABORT("exception_ptr_cast was called in -fno-exceptions mode");
#  endif
}

template <class _Ep>
void exception_ptr_cast(const exception_ptr&&) = delete;
#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_UNVERSIONED_NAMESPACE_STD

#undef _LIBCPP_CONSTEXPR_EXCEPTION_PTR
#undef _LIBCPP_HAS_CONSTEXPR_EXCEPTION_PTR

_LIBCPP_POP_MACROS

#endif // _LIBCPP___EXCEPTION_EXCEPTION_PTR_H
