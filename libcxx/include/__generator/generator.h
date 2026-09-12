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

template <class _Ref, class _Val = void, class _Allocator = void>
class generator : public ranges::view_interface<generator<_Ref, _Val, _Allocator> > {
  template <class, class, class>
  friend class generator;
  using __value     = conditional_t<is_void_v<_Val>, remove_cvref_t<_Ref>, _Val>;
  using __reference = conditional_t<is_void_v<_Val>, _Ref&&, _Ref>;

  struct __state {
    coroutine_handle<> __parent_ = nullptr;
    coroutine_handle<> __top_ = nullptr;
    void* __value_ = nullptr;
    exception_ptr __exception_;
  };

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
    __state* __state_ = nullptr;
    friend class generator;
    struct __final_awaiter {
      bool await_ready() noexcept { return false; }
      coroutine_handle<> await_suspend(coroutine_handle<promise_type> __h) noexcept {
        __state* __s = __h.promise().__state_;
        if (__s && __s->__parent_) { __s->__top_ = __s->__parent_; return __s->__parent_; }
        return noop_coroutine();
      }
      void await_resume() noexcept {}
    };
    struct __recursive_awaiter {
      generator __nested_;
      bool await_ready() noexcept { return false; }
      coroutine_handle<> await_suspend(coroutine_handle<promise_type> __parent) noexcept {
        __state* __s = __parent.promise().__state_;
        auto __child = __nested_.__coroutine_;
        __child.promise().__state_ = __s;
        __s->__parent_ = __parent;
        __s->__top_ = __child;
        return __child;
      }
      void await_resume() {
        __nested_.__coroutine_.promise().__state_->__parent_ = nullptr;
        if (__nested_.__coroutine_.promise().__state_->__exception_)
          rethrow_exception(__nested_.__coroutine_.promise().__state_->__exception_);
      }
    };
  public:
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
      return __recursive_awaiter{std::move(__r.range)};
    }
    void return_void() const noexcept {}
    void await_transform() = delete;
    _LIBCPP_HIDE_FROM_ABI void unhandled_exception() { __state_->__exception_ = current_exception(); }
  };

private:
  explicit generator(coroutine_handle<promise_type> __c) noexcept : __coroutine_(__c), __state_(make_unique<__state>()) {}
  coroutine_handle<promise_type> __coroutine_ = nullptr;
  unique_ptr<__state> __state_;
};

namespace pmr {
template <class _Ref, class _Val = void>
using generator = std::generator<_Ref, _Val, polymorphic_allocator<> >;
} // namespace pmr

#endif
_LIBCPP_END_NAMESPACE_STD
#endif
