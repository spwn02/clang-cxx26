//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___IO_STREAM_ERASED_H
#define _LIBCPP___IO_STREAM_ERASED_H

#include <__assert>
#include <__config>
#include <__coroutine/coroutine_handle.h>
#include <__coroutine/noop_coroutine_handle.h>
#include <__cstddef/size_t.h>
#include <__io/io_env.h>
#include <__io_buffer/buffer.h>
#include <__io_buffer/buffer_sequence.h>
#include <__type_traits/is_same.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/exchange.h>
#include <__utility/integer_sequence.h>
#include <__utility/move.h>
#include <__new/allocate.h>
#include <__new/placement_new_delete.h>
#include <__type_traits/is_void.h>
#include <__tuple/tuple_element.h>
#include <__tuple/tuple_size.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

#if _LIBCPP_HAS_EXPERIMENTAL_NETWORKING && _LIBCPP_HAS_THREADS

_LIBCPP_BEGIN_NAMESPACE_STD

namespace io {

// The type-erasing wrappers copy the caller's buffer sequence into a fixed array inside the wrapper (the operation may
// outlive the call that created it, and the wrapped stream sees a span over this array). A partial operation
// (read_some, write_some) simply sees the first __erased_max_buffers non-empty buffers, which it is allowed to do; a
// complete operation (read, write) requires that many or fewer, because a larger gather would take a heap allocation
// per operation.
inline constexpr size_t __erased_max_buffers = 16;

// Copies up to __erased_max_buffers non-empty buffers of the sequence into __out, after skipping __skip bytes. Returns the
// number copied.
template <class _Buf, class _Seq>
_LIBCPP_HIDE_FROM_ABI size_t __gather_buffers(const _Seq& __seq, size_t __skip, _Buf* __out) noexcept {
  size_t __n  = 0;
  auto __it   = io::buffer_sequence_begin(__seq);
  auto __last = io::buffer_sequence_end(__seq);
  for (; __it != __last && __n < __erased_max_buffers; ++__it) {
    _Buf __b = static_cast<_Buf>(*__it);
    if (__skip != 0) {
      size_t __s = __skip < __b.size() ? __skip : __b.size();
      __b += __s;
      __skip -= __s;
    }
    if (__b.size() != 0)
      __out[__n++] = __b;
  }
  return __n;
}

// True if the sequence has more than __erased_max_buffers non-empty buffers.
template <class _Seq>
_LIBCPP_HIDE_FROM_ABI bool __exceeds_erased_max(const _Seq& __seq) noexcept {
  size_t __count = 0;
  for (auto __it = io::buffer_sequence_begin(__seq), __last = io::buffer_sequence_end(__seq); __it != __last; ++__it)
    if (static_cast<const_buffer>(*__it).size() != 0)
      ++__count;
  return __count > __erased_max_buffers;
}

// Per-operation dispatch, normalized: the wrapped awaitable's await_suspend may return void, bool or a handle.
template <class _Res>
struct __erased_op_vtable {
  bool (*__ready_)(void*);
  coroutine_handle<> (*__suspend_)(void*, coroutine_handle<>, const io_env*);
  _Res (*__resume_)(void*);
  void (*__destroy_)(void*) noexcept;
};

template <class _Res, class _Aw>
inline constexpr __erased_op_vtable<_Res> __erased_op_vtable_for = {
    [](void* __p) -> bool { return static_cast<_Aw*>(__p)->await_ready(); },
    [](void* __p, coroutine_handle<> __h, const io_env* __env) -> coroutine_handle<> {
      using _Ret = decltype(static_cast<_Aw*>(__p)->await_suspend(__h, __env));
      if constexpr (is_void_v<_Ret>) {
        static_cast<_Aw*>(__p)->await_suspend(__h, __env);
        return noop_coroutine();
      } else if constexpr (is_same_v<_Ret, bool>) {
        return static_cast<_Aw*>(__p)->await_suspend(__h, __env) ? coroutine_handle<>(noop_coroutine()) : __h;
      } else {
        return static_cast<_Aw*>(__p)->await_suspend(__h, __env);
      }
    },
    [](void* __p) -> _Res {
      auto&& __r = static_cast<_Aw*>(__p)->await_resume();
      return []<size_t... _Is>(auto&& __x, index_sequence<_Is...>) -> _Res {
        using std::get;
        return _Res(get<_Is>(std::move(__x))...);
      }(std::move(__r), make_index_sequence<tuple_size_v<_Res>>{});
    },
    [](void* __p) noexcept { static_cast<_Aw*>(__p)->~_Aw(); }};

// The preallocated storage for the one operation a wrapper may have in flight.
class __erased_slot {
  void* __mem_                          = nullptr;
  size_t __size_                        = 0;
  size_t __align_                       = 0;
  void (*__destroy_active_)(void*) noexcept = nullptr; // non-null while an operation is alive

public:
  _LIBCPP_HIDE_FROM_ABI __erased_slot() noexcept = default;
  _LIBCPP_HIDE_FROM_ABI __erased_slot(size_t __size, size_t __align)
      : __mem_(std::__libcpp_allocate<char>(__element_count(__size), __align)), __size_(__size), __align_(__align) {}
  __erased_slot(const __erased_slot&)            = delete;
  __erased_slot& operator=(const __erased_slot&) = delete;
  _LIBCPP_HIDE_FROM_ABI __erased_slot(__erased_slot&& __o) noexcept
      : __mem_(std::exchange(__o.__mem_, nullptr)),
        __size_(__o.__size_),
        __align_(__o.__align_),
        __destroy_active_(std::exchange(__o.__destroy_active_, nullptr)) {}
  _LIBCPP_HIDE_FROM_ABI __erased_slot& operator=(__erased_slot&& __o) noexcept {
    if (this != &__o) {
      __release();
      __mem_            = std::exchange(__o.__mem_, nullptr);
      __size_           = __o.__size_;
      __align_          = __o.__align_;
      __destroy_active_ = std::exchange(__o.__destroy_active_, nullptr);
    }
    return *this;
  }
  _LIBCPP_HIDE_FROM_ABI ~__erased_slot() { __release(); }

  _LIBCPP_HIDE_FROM_ABI void* __memory() const noexcept { return __mem_; }
  _LIBCPP_HIDE_FROM_ABI bool __active() const noexcept { return __destroy_active_ != nullptr; }
  _LIBCPP_HIDE_FROM_ABI void __mark_active(void (*__d)(void*) noexcept) noexcept { __destroy_active_ = __d; }
  _LIBCPP_HIDE_FROM_ABI void __end() noexcept {
    if (void (*__d)(void*) noexcept = std::exchange(__destroy_active_, nullptr))
      __d(__mem_);
  }

private:
  _LIBCPP_HIDE_FROM_ABI void __release() noexcept {
    __end();
    if (__mem_) {
      std::__libcpp_deallocate<char>(static_cast<char*>(__mem_), __element_count(__size_), __align_);
      __mem_ = nullptr;
    }
  }
};

// What a wrapper's operation returns. The operation state lives in the wrapper's slot; this object is a reference to
// it. Destroying the object before await_resume ends the operation.
template <class _Res>
class __erased_awaitable {
  __erased_slot* __slot_;
  const __erased_op_vtable<_Res>* __op_;

public:
  _LIBCPP_HIDE_FROM_ABI __erased_awaitable(__erased_slot* __s, const __erased_op_vtable<_Res>* __op) noexcept
      : __slot_(__s), __op_(__op) {}
  _LIBCPP_HIDE_FROM_ABI __erased_awaitable(__erased_awaitable&& __o) noexcept
      : __slot_(std::exchange(__o.__slot_, nullptr)), __op_(__o.__op_) {}
  __erased_awaitable& operator=(const __erased_awaitable&) = delete;
  _LIBCPP_HIDE_FROM_ABI ~__erased_awaitable() {
    if (__slot_)
      __slot_->__end();
  }

  _LIBCPP_HIDE_FROM_ABI bool await_ready() { return __op_->__ready_(__slot_->__memory()); }
  _LIBCPP_HIDE_FROM_ABI coroutine_handle<> await_suspend(coroutine_handle<> __h, const io_env* __env) {
    return __op_->__suspend_(__slot_->__memory(), __h, __env);
  }
  _LIBCPP_HIDE_FROM_ABI _Res await_resume() {
    struct __guard {
      __erased_slot* __s_;
      _LIBCPP_HIDE_FROM_ABI ~__guard() { __s_->__end(); }
    } __g{std::exchange(__slot_, nullptr)};
    return __op_->__resume_(__g.__s_->__memory());
  }
};

// Constructs the operation _Aw (the result of calling the wrapped object) in the slot's memory, and returns its dispatch table.
template <class _Res, class _Aw, class _Make>
_LIBCPP_HIDE_FROM_ABI const __erased_op_vtable<_Res>* __erased_construct(void* __mem, _Make&& __make) {
  ::new (__mem) _Aw(__make());
  return &__erased_op_vtable_for<_Res, _Aw>;
}

// Marks the operation just constructed as active and hands out the awaitable that drives it.
template <class _Res>
_LIBCPP_HIDE_FROM_ABI __erased_awaitable<_Res>
__erased_begin(__erased_slot& __slot, const __erased_op_vtable<_Res>* __op) noexcept {
  __slot.__mark_active(__op->__destroy_);
  return __erased_awaitable<_Res>(&__slot, __op);
}

// The part of a wrapper that is the same for all seven: the object pointer, the vtable pointer and the slot. A wrapper
// that has an operation in flight can be neither moved nor destroyed from under it (the destructor ends the operation).
template <class _Vt>
class __erased_state {
public:
  void* __obj_       = nullptr;
  const _Vt* __vt_   = nullptr;
  __erased_slot __slot_;

  _LIBCPP_HIDE_FROM_ABI __erased_state() noexcept = default;
  _LIBCPP_HIDE_FROM_ABI __erased_state(void* __obj, const _Vt* __vt)
      : __obj_(__obj), __vt_(__vt), __slot_(__vt->__size_, __vt->__align_) {}
  __erased_state(const __erased_state&)            = delete;
  __erased_state& operator=(const __erased_state&) = delete;
  _LIBCPP_HIDE_FROM_ABI __erased_state(__erased_state&& __o) noexcept
      : __obj_(std::exchange(__o.__obj_, nullptr)),
        __vt_(std::exchange(__o.__vt_, nullptr)),
        __slot_(std::move(__o.__slot_)) {
    _LIBCPP_ASSERT_UNCATEGORIZED(!__slot_.__active(), "moving a type-erased stream with an operation in flight");
  }
  _LIBCPP_HIDE_FROM_ABI __erased_state& operator=(__erased_state&& __o) noexcept {
    if (this != &__o) {
      _LIBCPP_ASSERT_UNCATEGORIZED(!__o.__slot_.__active(), "moving a type-erased stream with an operation in flight");
      __obj_  = std::exchange(__o.__obj_, nullptr);
      __vt_   = std::exchange(__o.__vt_, nullptr);
      __slot_ = std::move(__o.__slot_);
    }
    return *this;
  }
};

template <class... _Ts>
inline constexpr size_t __max_sizeof = 0;
template <class _T0, class... _Ts>
inline constexpr size_t __max_sizeof<_T0, _Ts...> =
    sizeof(_T0) > __max_sizeof<_Ts...> ? sizeof(_T0) : __max_sizeof<_Ts...>;
template <class... _Ts>
inline constexpr size_t __max_alignof = 1;
template <class _T0, class... _Ts>
inline constexpr size_t __max_alignof<_T0, _Ts...> =
    alignof(_T0) > __max_alignof<_Ts...> ? alignof(_T0) : __max_alignof<_Ts...>;

} // namespace io

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP_HAS_EXPERIMENTAL_NETWORKING && _LIBCPP_HAS_THREADS

#endif // _LIBCPP___IO_STREAM_ERASED_H
