// -*- C++ -*-
//===----------------------------------------------------------------------===//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___GENERATOR_GENERATOR_H
#define _LIBCPP___GENERATOR_GENERATOR_H

#include <__config>
#include <coroutine>
#include <exception>
#include <memory>
#include <memory_resource>
#include <ranges>
#include <type_traits>
#include <utility>

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 23

struct __generator_state {
  coroutine_handle<> __parent_ = nullptr;
  coroutine_handle<> __top_ = nullptr;
  void* __value_ = nullptr;
  exception_ptr __exception_;
};

template <class _Ref, class _Val = void, class _Allocator = void>
class generator : public ranges::view_interface<generator<_Ref, _Val, _Allocator> > {
  template <class, class, class>
  friend class generator;
  using __value     = conditional_t<is_void_v<_Val>, remove_cvref_t<_Ref>, _Val>;
  using __reference = conditional_t<is_void_v<_Val>, _Ref&&, _Ref>;

public:
  using yielded = conditional_t<is_reference_v<__reference>, __reference, const __reference&>;

  class promise_type;
  class iterator;

  generator(const generator&) = delete;
  _LIBCPP_HIDE_FROM_ABI generator(generator&& __other) noexcept
      : __coroutine_(exchange(__other.__coroutine_, {})), __state_(exchange(__other.__state_, nullptr)) {}
  _LIBCPP_HIDE_FROM_ABI ~generator() { if (__coroutine_) __coroutine_.destroy(); }
  _LIBCPP_HIDE_FROM_ABI generator& operator=(generator __other) noexcept {
    swap(__coroutine_, __other.__coroutine_); swap(__state_, __other.__state_); return *this;
  }

  _LIBCPP_HIDE_FROM_ABI iterator begin() {
    __state_->__top_ = __coroutine_;
    __coroutine_.promise().__state_ = __state_.get();
    __coroutine_.resume();
    if (__state_->__exception_) rethrow_exception(__state_->__exception_);
    return iterator(__coroutine_);
  }
  _LIBCPP_HIDE_FROM_ABI default_sentinel_t end() const noexcept { return {}; }

  class iterator {
    coroutine_handle<promise_type> __coroutine_ = nullptr;
    explicit iterator(coroutine_handle<promise_type> __c) : __coroutine_(__c) {}
    friend class generator;
  public:
    using value_type = __value;
    using difference_type = ptrdiff_t;
    iterator() = default;
    iterator(const iterator&) = delete;
    _LIBCPP_HIDE_FROM_ABI iterator(iterator&&) noexcept = default;
    _LIBCPP_HIDE_FROM_ABI iterator& operator=(iterator&&) noexcept = default;
    _LIBCPP_HIDE_FROM_ABI __reference operator*() const noexcept { return static_cast<__reference>(*static_cast<add_pointer_t<yielded>>(__coroutine_.promise().__state_->__value_)); }
    _LIBCPP_HIDE_FROM_ABI iterator& operator++() {
      __coroutine_.promise().__state_->__top_.resume();
      if (__coroutine_.promise().__state_->__exception_) rethrow_exception(__coroutine_.promise().__state_->__exception_);
      return *this;
    }
    _LIBCPP_HIDE_FROM_ABI void operator++(int) { ++*this; }
    friend _LIBCPP_HIDE_FROM_ABI bool operator==(const iterator& __i, default_sentinel_t) noexcept {
      return !__i.__coroutine_ || __i.__coroutine_.done();
    }
  };

  class promise_type {
    // [coro.generator.promise] allocates coroutine states in units whose size
    // and alignment are both the default new alignment.  Keep an erased
    // deallocation record after the coroutine state: operator delete is only
    // given the original state size, and (when _Allocator is void) the
    // allocator passed through allocator_arg can have an arbitrary type.
    struct alignas(__STDCPP_DEFAULT_NEW_ALIGNMENT__) __allocation_unit {
      unsigned char __data_[__STDCPP_DEFAULT_NEW_ALIGNMENT__];
    };
    struct __allocation_header {
      size_t __count_;
      size_t __allocator_offset_;
      void (*__deallocate_)(void*, const __allocation_header*) noexcept;
    };

    _LIBCPP_HIDE_FROM_ABI static constexpr size_t __align_up(size_t __n, size_t __alignment) noexcept {
      return (__n + __alignment - 1) / __alignment * __alignment;
    }
    _LIBCPP_HIDE_FROM_ABI static constexpr size_t __header_offset(size_t __size) noexcept {
      return __align_up(__size, alignof(__allocation_header));
    }

    template <class _Alloc>
    _LIBCPP_HIDE_FROM_ABI static void __deallocate(void* __pointer, const __allocation_header* __header) noexcept {
      using _B = __allocator_traits_rebind_t<_Alloc, __allocation_unit>;
      auto* __stored = reinterpret_cast<_B*>(
          static_cast<unsigned char*>(__pointer) + __header->__allocator_offset_);
      // Cpp17Allocator copy construction does not throw.  The copy must
      // outlive the allocator object embedded in the allocation it releases.
      _B __alloc(*__stored);
      __stored->~_B();
      size_t __count = __header->__count_;
      const_cast<__allocation_header*>(__header)->~__allocation_header();
      allocator_traits<_B>::deallocate(__alloc, static_cast<__allocation_unit*>(__pointer), __count);
    }

    template <class _Alloc>
    _LIBCPP_HIDE_FROM_ABI static void* __allocate(size_t __size, const _Alloc& __input_alloc) {
      using _A = conditional_t<is_void_v<_Allocator>, _Alloc, _Allocator>;
      using _B = __allocator_traits_rebind_t<_A, __allocation_unit>;
      static_assert(is_pointer_v<typename allocator_traits<_B>::pointer>);

      _A __a(__input_alloc);
      _B __b(__a);
      size_t __header_pos = __header_offset(__size);
      size_t __allocator_begin = __header_pos + sizeof(__allocation_header);
      size_t __count = __align_up(__allocator_begin + alignof(_B) - 1 + sizeof(_B), sizeof(__allocation_unit)) /
                       sizeof(__allocation_unit);
      __allocation_unit* __pointer = allocator_traits<_B>::allocate(__b, __count);
      void* __allocator_address = reinterpret_cast<unsigned char*>(__pointer) + __allocator_begin;
      size_t __allocator_space  = __count * sizeof(__allocation_unit) - __allocator_begin;
      (void)std::align(alignof(_B), sizeof(_B), __allocator_address, __allocator_space);
      size_t __allocator_pos = static_cast<unsigned char*>(__allocator_address) -
                               reinterpret_cast<unsigned char*>(__pointer);
#  if _LIBCPP_HAS_EXCEPTIONS
      try {
#  endif
        ::new (static_cast<void*>(reinterpret_cast<unsigned char*>(__pointer) + __allocator_pos)) _B(__b);
#  if _LIBCPP_HAS_EXCEPTIONS
      } catch (...) {
        allocator_traits<_B>::deallocate(__b, __pointer, __count);
        throw;
      }
#  endif
      ::new (static_cast<void*>(reinterpret_cast<unsigned char*>(__pointer) + __header_pos))
          __allocation_header{__count, __allocator_pos, &__deallocate<_A>};
      return __pointer;
    }

    __generator_state* __state_ = nullptr;
    friend class generator;
    struct __final_awaiter {
      bool await_ready() noexcept { return false; }
      coroutine_handle<> await_suspend(coroutine_handle<promise_type> __h) noexcept {
        __generator_state* __s = __h.promise().__state_;
        if (__s && __s->__parent_) { __s->__top_ = __s->__parent_; return __s->__parent_; }
        return noop_coroutine();
      }
      void await_resume() noexcept {}
    };
    template <class _NestedGenerator>
    struct __recursive_awaiter {
      _NestedGenerator __nested_;
      __generator_state* __state_ = nullptr;
      coroutine_handle<> __previous_parent_ = nullptr;
      bool await_ready() noexcept { return false; }
      coroutine_handle<> await_suspend(coroutine_handle<promise_type> __parent) noexcept {
        __generator_state* __s = __parent.promise().__state_;
        auto __child = __nested_.__coroutine_;
        __state_ = __s;
        __child.promise().__set_state(__s);
        __previous_parent_ = __s->__parent_;
        __s->__parent_ = __parent;
        __s->__top_ = __child;
        return __child;
      }
      void await_resume() {
        exception_ptr __exception = exchange(__state_->__exception_, {});
        __state_->__parent_ = __previous_parent_;
        if (__exception)
          rethrow_exception(__exception);
      }
    };
  public:
    _LIBCPP_HIDE_FROM_ABI void __set_state(__generator_state* __state) noexcept { __state_ = __state; }
    _LIBCPP_HIDE_FROM_ABI generator get_return_object() noexcept {
      return generator(coroutine_handle<promise_type>::from_promise(*this));
    }
    _LIBCPP_HIDE_FROM_ABI suspend_always initial_suspend() const noexcept { return {}; }
    _LIBCPP_HIDE_FROM_ABI __final_awaiter final_suspend() noexcept { return {}; }
    _LIBCPP_HIDE_FROM_ABI suspend_always yield_value(yielded __value) noexcept {
      // __value_ is an untyped void* (it has to be, since it's shared state
      // between arbitrarily-nested generator instantiations with different
      // `yielded` types) -- for a const-qualified `yielded` (e.g.
      // `generator<const T&>`), addressof(__value) is a `const T*`, which
      // must have its constness stripped to store here. This does not
      // create a genuine const-correctness hole: nothing ever writes
      // through this pointer, only reads it back via operator*() with the
      // original (correctly const-qualified) `yielded`/`__reference` type.
      __state_->__value_ = const_cast<void*>(static_cast<const void*>(addressof(__value)));
      return {};
    }

    // [coro.generator.promise]: when `yielded` is an rvalue reference (the
    // common case, e.g. `yielded == int&&` for `generator<int>`), the above
    // overload can't bind an lvalue argument (e.g. `co_yield i;` for a named
    // local `int i`) -- the standard requires this second overload, which
    // copy-constructs a value that lives inside the returned awaiter (so it
    // survives the suspension point) and points __value_ at that stored
    // copy, not at the original (potentially short-lived) lvalue.
    struct __copy_awaiter {
      remove_cvref_t<yielded> __val_;
      _LIBCPP_HIDE_FROM_ABI static constexpr bool await_ready() noexcept { return false; }
      _LIBCPP_HIDE_FROM_ABI bool await_suspend(coroutine_handle<promise_type> __h) noexcept {
        __h.promise().__state_->__value_ = addressof(__val_);
        return true;
      }
      _LIBCPP_HIDE_FROM_ABI void await_resume() const noexcept {}
    };
    _LIBCPP_HIDE_FROM_ABI auto yield_value(const remove_reference_t<yielded>& __lval)
      requires is_rvalue_reference_v<yielded> &&
               constructible_from<remove_cvref_t<yielded>, const remove_reference_t<yielded>&>
    {
      return __copy_awaiter{__lval};
    }

    template <class _R2, class _V2, class _A2, class _Unused>
      requires same_as<typename generator<_R2, _V2, _A2>::yielded, yielded>
    _LIBCPP_HIDE_FROM_ABI auto yield_value(ranges::elements_of<generator<_R2, _V2, _A2>&&, _Unused> __r) noexcept {
      return __recursive_awaiter<generator<_R2, _V2, _A2>>{std::move(__r.range)};
    }
    template <class _R2, class _V2, class _A2, class _Unused>
      requires same_as<typename generator<_R2, _V2, _A2>::yielded, yielded>
    _LIBCPP_HIDE_FROM_ABI auto yield_value(ranges::elements_of<generator<_R2, _V2, _A2>&, _Unused> __r) noexcept {
      return __recursive_awaiter<generator<_R2, _V2, _A2>>{std::move(__r.range)};
    }
    // LWG 4418: the published convertible_to constraint rejects the motivating
    // generator<int>/vector<int> case.  Constrain this exactly as the loop body
    // is used: an element must be accepted by one of the ordinary yield_value
    // overloads above.
    template <ranges::input_range _Range, class _Alloc>
      requires requires(promise_type& __promise, ranges::iterator_t<_Range>& __iterator) {
        __promise.yield_value(*__iterator);
      }
    _LIBCPP_HIDE_FROM_ABI auto yield_value(ranges::elements_of<_Range, _Alloc> __r) {
      auto __nested = [](allocator_arg_t,
                         _Alloc,
                         ranges::iterator_t<_Range> __first,
                         ranges::sentinel_t<_Range> __last) -> generator<yielded, void, _Alloc> {
        for (; __first != __last; ++__first)
          co_yield *__first;
      };
      return yield_value(ranges::elements_of(
          __nested(allocator_arg, __r.allocator, ranges::begin(__r.range), ranges::end(__r.range))));
    }
    void return_void() const noexcept {}
    void await_transform() = delete;
    _LIBCPP_HIDE_FROM_ABI void unhandled_exception() { __state_->__exception_ = current_exception(); }

    _LIBCPP_HIDE_FROM_ABI static void* operator new(size_t __size)
      requires same_as<_Allocator, void> || default_initializable<_Allocator>
    {
      if constexpr (is_void_v<_Allocator>)
        return __allocate(__size, allocator<byte>());
      else
        return __allocate(__size, _Allocator());
    }
    template <class _Alloc, class... _Args>
    _LIBCPP_HIDE_FROM_ABI static void*
    operator new(size_t __size, allocator_arg_t, const _Alloc& __alloc, const _Args&...) {
      static_assert(same_as<_Allocator, void> || convertible_to<const _Alloc&, _Allocator>);
      return __allocate(__size, __alloc);
    }
    template <class _This, class _Alloc, class... _Args>
    _LIBCPP_HIDE_FROM_ABI static void*
    operator new(size_t __size, const _This&, allocator_arg_t, const _Alloc& __alloc, const _Args&...) {
      static_assert(same_as<_Allocator, void> || convertible_to<const _Alloc&, _Allocator>);
      return __allocate(__size, __alloc);
    }
    _LIBCPP_HIDE_FROM_ABI static void operator delete(void* __pointer, size_t __size) noexcept {
      auto* __header = reinterpret_cast<const __allocation_header*>(
          static_cast<unsigned char*>(__pointer) + __header_offset(__size));
      __header->__deallocate_(__pointer, __header);
    }
  };

private:
  explicit generator(coroutine_handle<promise_type> __c) noexcept
      : __coroutine_(__c), __state_(make_unique<__generator_state>()) {}
  coroutine_handle<promise_type> __coroutine_ = nullptr;
  unique_ptr<__generator_state> __state_;
};

namespace pmr {
template <class _Ref, class _Val = void>
using generator = std::generator<_Ref, _Val, polymorphic_allocator<> >;
} // namespace pmr

#endif
_LIBCPP_END_NAMESPACE_STD
#endif
