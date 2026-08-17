// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Experimental C++26 std::polymorphic implementation for the Bloomberg libc++ fork.
// P3019R14: indirect and polymorphic: Vocabulary Types for Composite Class Design.
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___MEMORY_POLYMORPHIC_H
#define _LIBCPP___MEMORY_POLYMORPHIC_H

#include <__config>
#include <__memory/allocator.h>
#include <__memory/allocator_traits.h>
#include <__utility/forward.h>
#include <__utility/in_place.h>
#include <__type_traits/is_base_of.h>
#include <__type_traits/is_constructible.h>
#include <__type_traits/is_same.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/move.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_PUSH_MACROS
#include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

// polymorphic<T, Allocator> owns an object of some type derived from (or
// equal to) T without ever exposing its dynamic type. Unlike indirect<T>,
// copying a polymorphic<T> must copy the *actual* derived object, so the
// control block below type-erases just enough (clone/destroy) to do that
// through a manually-managed vtable-equivalent, dispatched via ordinary
// virtual functions.
template <class _Tp, class _Allocator = allocator<_Tp>>
class polymorphic {
  using _AllocTraits = allocator_traits<_Allocator>;

  struct __control_block {
    _LIBCPP_HIDE_FROM_ABI virtual ~__control_block()                                  = default;
    _LIBCPP_HIDE_FROM_ABI virtual _Tp* __get() noexcept                               = 0;
    _LIBCPP_HIDE_FROM_ABI virtual __control_block* __clone(const _Allocator&) const   = 0;
    _LIBCPP_HIDE_FROM_ABI virtual void __destroy(_Allocator&)                         = 0;
  };

  template <class _Up>
  struct __control_block_impl final : __control_block {
    _Up __value_;

    template <class... _Args>
    _LIBCPP_HIDE_FROM_ABI explicit __control_block_impl(_Args&&... __args) : __value_(std::forward<_Args>(__args)...) {}

    _LIBCPP_HIDE_FROM_ABI _Tp* __get() noexcept override { return std::addressof(__value_); }

    _LIBCPP_HIDE_FROM_ABI __control_block* __clone(const _Allocator& __a) const override {
      using _Rebound = typename _AllocTraits::template rebind_alloc<__control_block_impl>;
      using _RTraits  = allocator_traits<_Rebound>;
      _Rebound __ra(__a);
      auto* __mem = _RTraits::allocate(__ra, 1);
      _RTraits::construct(__ra, __mem, __value_);
      return __mem;
    }

    _LIBCPP_HIDE_FROM_ABI void __destroy(_Allocator& __a) override {
      using _Rebound = typename _AllocTraits::template rebind_alloc<__control_block_impl>;
      using _RTraits  = allocator_traits<_Rebound>;
      _Rebound __ra(__a);
      _RTraits::destroy(__ra, this);
      _RTraits::deallocate(__ra, this, 1);
    }
  };

  template <class _Up, class... _Args>
  _LIBCPP_HIDE_FROM_ABI __control_block* __make_cb(_Args&&... __args) {
    using _Rebound = typename _AllocTraits::template rebind_alloc<__control_block_impl<_Up>>;
    using _RTraits  = allocator_traits<_Rebound>;
    _Rebound __ra(alloc_);
    auto* __mem = _RTraits::allocate(__ra, 1);
    _RTraits::construct(__ra, __mem, std::forward<_Args>(__args)...);
    return __mem;
  }

  _LIBCPP_HIDE_FROM_ABI void __reset() {
    if (cb_ != nullptr) {
      cb_->__destroy(alloc_);
      cb_ = nullptr;
    }
  }

public:
  using value_type     = _Tp;
  using allocator_type = _Allocator;

  _LIBCPP_HIDE_FROM_ABI explicit polymorphic()
    requires is_default_constructible_v<_Tp>
  {
    cb_ = __make_cb<_Tp>();
  }

  _LIBCPP_HIDE_FROM_ABI explicit polymorphic(allocator_arg_t, const _Allocator& __a)
    requires is_default_constructible_v<_Tp>
      : alloc_(__a) {
    cb_ = __make_cb<_Tp>();
  }

  _LIBCPP_HIDE_FROM_ABI polymorphic(const polymorphic& __other)
      : alloc_(_AllocTraits::select_on_container_copy_construction(__other.alloc_)) {
    cb_ = __other.valueless_after_move() ? nullptr : __other.cb_->__clone(alloc_);
  }

  _LIBCPP_HIDE_FROM_ABI polymorphic(allocator_arg_t, const _Allocator& __a, const polymorphic& __other) : alloc_(__a) {
    cb_ = __other.valueless_after_move() ? nullptr : __other.cb_->__clone(alloc_);
  }

  _LIBCPP_HIDE_FROM_ABI polymorphic(polymorphic&& __other) noexcept
      : alloc_(std::move(__other.alloc_)), cb_(__other.cb_) {
    __other.cb_ = nullptr;
  }

  _LIBCPP_HIDE_FROM_ABI polymorphic(allocator_arg_t, const _Allocator& __a, polymorphic&& __other) noexcept(
      _AllocTraits::is_always_equal::value)
      : alloc_(__a) {
    if (__other.valueless_after_move()) {
      cb_ = nullptr;
    } else if (alloc_ == __other.alloc_) {
      cb_         = __other.cb_;
      __other.cb_ = nullptr;
    } else {
      cb_ = __other.cb_->__clone(alloc_);
      __other.__reset();
    }
  }

  template <class _Up = _Tp>
    requires(!is_same_v<remove_cvref_t<_Up>, polymorphic> && is_constructible_v<_Tp, _Up>)
  _LIBCPP_HIDE_FROM_ABI explicit polymorphic(_Up&& __u) {
    cb_ = __make_cb<remove_cvref_t<_Up>>(std::forward<_Up>(__u));
  }

  template <class _Up, class... _Ts>
    requires is_base_of_v<_Tp, _Up> && is_constructible_v<_Up, _Ts...>
  _LIBCPP_HIDE_FROM_ABI explicit polymorphic(in_place_type_t<_Up>, _Ts&&... __ts) {
    cb_ = __make_cb<_Up>(std::forward<_Ts>(__ts)...);
  }

  template <class _Up, class... _Ts>
    requires is_base_of_v<_Tp, _Up> && is_constructible_v<_Up, _Ts...>
  _LIBCPP_HIDE_FROM_ABI explicit polymorphic(allocator_arg_t, const _Allocator& __a, in_place_type_t<_Up>, _Ts&&... __ts)
      : alloc_(__a) {
    cb_ = __make_cb<_Up>(std::forward<_Ts>(__ts)...);
  }

  _LIBCPP_HIDE_FROM_ABI ~polymorphic() { __reset(); }

  _LIBCPP_HIDE_FROM_ABI polymorphic& operator=(const polymorphic& __other) {
    if (this == std::addressof(__other))
      return *this;
    __control_block* __new_cb =
        __other.valueless_after_move()
            ? nullptr
            : __other.cb_->__clone(_AllocTraits::propagate_on_container_copy_assignment::value ? __other.alloc_ : alloc_);
    __reset();
    if constexpr (_AllocTraits::propagate_on_container_copy_assignment::value)
      alloc_ = __other.alloc_;
    cb_ = __new_cb;
    return *this;
  }

  _LIBCPP_HIDE_FROM_ABI polymorphic& operator=(polymorphic&& __other) noexcept(
      _AllocTraits::propagate_on_container_move_assignment::value || _AllocTraits::is_always_equal::value) {
    if (this == std::addressof(__other))
      return *this;
    __reset();
    if constexpr (_AllocTraits::propagate_on_container_move_assignment::value)
      alloc_ = std::move(__other.alloc_);
    if (__other.valueless_after_move()) {
      cb_ = nullptr;
    } else if (alloc_ == __other.alloc_) {
      cb_         = __other.cb_;
      __other.cb_ = nullptr;
    } else {
      cb_ = __other.cb_->__clone(alloc_);
      __other.__reset();
    }
    return *this;
  }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI const _Tp& operator*() const& noexcept { return *cb_->__get(); }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI _Tp& operator*() & noexcept { return *cb_->__get(); }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI const _Tp&& operator*() const&& noexcept { return std::move(*cb_->__get()); }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI _Tp&& operator*() && noexcept { return std::move(*cb_->__get()); }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI const _Tp* operator->() const noexcept { return cb_->__get(); }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI _Tp* operator->() noexcept { return cb_->__get(); }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI bool valueless_after_move() const noexcept { return cb_ == nullptr; }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI allocator_type get_allocator() const noexcept { return alloc_; }

  _LIBCPP_HIDE_FROM_ABI void swap(polymorphic& __other) noexcept(
      _AllocTraits::propagate_on_container_swap::value || _AllocTraits::is_always_equal::value) {
    using std::swap;
    if constexpr (_AllocTraits::propagate_on_container_swap::value)
      swap(alloc_, __other.alloc_);
    swap(cb_, __other.cb_);
  }

  _LIBCPP_HIDE_FROM_ABI friend void swap(polymorphic& __x, polymorphic& __y) noexcept(noexcept(__x.swap(__y))) {
    __x.swap(__y);
  }

private:
  [[no_unique_address]] _Allocator alloc_{};
  __control_block* cb_ = nullptr;
};

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP___MEMORY_POLYMORPHIC_H
