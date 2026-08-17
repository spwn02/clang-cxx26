// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Experimental C++26 std::indirect implementation for the Bloomberg libc++ fork.
// P3019R14: indirect and polymorphic: Vocabulary Types for Composite Class Design.
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___MEMORY_INDIRECT_H
#define _LIBCPP___MEMORY_INDIRECT_H

#include <__config>
#include <__functional/hash.h>
#include <__memory/allocator.h>
#include <__memory/allocator_traits.h>
#include <__utility/forward.h>
#include <__utility/in_place.h>
#include <__utility/move.h>
#include <__type_traits/is_constructible.h>
#include <__type_traits/is_same.h>
#include <__type_traits/remove_cvref.h>
#include <compare>
#include <initializer_list>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_PUSH_MACROS
#include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

template <class _Tp, class _Allocator = allocator<_Tp>>
class indirect {
  using _AllocTraits = allocator_traits<_Allocator>;

public:
  using value_type     = _Tp;
  using allocator_type = _Allocator;
  using pointer         = typename _AllocTraits::pointer;
  using const_pointer   = typename _AllocTraits::const_pointer;

  _LIBCPP_HIDE_FROM_ABI constexpr indirect()
    requires is_default_constructible_v<_Tp> && is_default_constructible_v<_Allocator>
  {
    __ptr_ = __make(alloc_);
  }

  _LIBCPP_HIDE_FROM_ABI constexpr explicit indirect(allocator_arg_t, const _Allocator& __a)
    requires is_default_constructible_v<_Tp>
      : alloc_(__a) {
    __ptr_ = __make(alloc_);
  }

  _LIBCPP_HIDE_FROM_ABI constexpr indirect(const indirect& __other)
      : alloc_(_AllocTraits::select_on_container_copy_construction(__other.alloc_)) {
    __ptr_ = __other.valueless_after_move() ? nullptr : __make(alloc_, *__other);
  }

  _LIBCPP_HIDE_FROM_ABI constexpr indirect(allocator_arg_t, const _Allocator& __a, const indirect& __other)
      : alloc_(__a) {
    __ptr_ = __other.valueless_after_move() ? nullptr : __make(alloc_, *__other);
  }

  _LIBCPP_HIDE_FROM_ABI constexpr indirect(indirect&& __other) noexcept
      : alloc_(std::move(__other.alloc_)), __ptr_(__other.__ptr_) {
    __other.__ptr_ = nullptr;
  }

  _LIBCPP_HIDE_FROM_ABI constexpr indirect(allocator_arg_t, const _Allocator& __a, indirect&& __other) noexcept(
      _AllocTraits::is_always_equal::value)
      : alloc_(__a) {
    if (__other.valueless_after_move()) {
      __ptr_ = nullptr;
    } else if (alloc_ == __other.alloc_) {
      __ptr_         = __other.__ptr_;
      __other.__ptr_ = nullptr;
    } else {
      __ptr_ = __make(alloc_, std::move(*__other));
    }
  }

  template <class _Up = _Tp>
    requires(!is_same_v<remove_cvref_t<_Up>, indirect> && !is_same_v<remove_cvref_t<_Up>, in_place_t> &&
             is_constructible_v<_Tp, _Up>)
  _LIBCPP_HIDE_FROM_ABI constexpr explicit indirect(_Up&& __u) {
    __ptr_ = __make(alloc_, std::forward<_Up>(__u));
  }

  template <class _Up = _Tp>
    requires is_constructible_v<_Tp, _Up>
  _LIBCPP_HIDE_FROM_ABI constexpr explicit indirect(allocator_arg_t, const _Allocator& __a, _Up&& __u) : alloc_(__a) {
    __ptr_ = __make(alloc_, std::forward<_Up>(__u));
  }

  template <class... _Us>
    requires is_constructible_v<_Tp, _Us...>
  _LIBCPP_HIDE_FROM_ABI constexpr explicit indirect(in_place_t, _Us&&... __us) {
    __ptr_ = __make(alloc_, std::forward<_Us>(__us)...);
  }

  template <class... _Us>
    requires is_constructible_v<_Tp, _Us...>
  _LIBCPP_HIDE_FROM_ABI constexpr explicit indirect(allocator_arg_t, const _Allocator& __a, in_place_t, _Us&&... __us)
      : alloc_(__a) {
    __ptr_ = __make(alloc_, std::forward<_Us>(__us)...);
  }

  _LIBCPP_HIDE_FROM_ABI constexpr ~indirect() { __reset(); }

  _LIBCPP_HIDE_FROM_ABI constexpr indirect& operator=(const indirect& __other) {
    if (this == std::addressof(__other))
      return *this;
    if (__other.valueless_after_move()) {
      __reset();
    } else {
      pointer __new_ptr = __make(_AllocTraits::propagate_on_container_copy_assignment::value ? __other.alloc_ : alloc_, *__other);
      __reset();
      if constexpr (_AllocTraits::propagate_on_container_copy_assignment::value)
        alloc_ = __other.alloc_;
      __ptr_ = __new_ptr;
    }
    return *this;
  }

  _LIBCPP_HIDE_FROM_ABI constexpr indirect& operator=(indirect&& __other) noexcept(
      _AllocTraits::propagate_on_container_move_assignment::value || _AllocTraits::is_always_equal::value) {
    if (this == std::addressof(__other))
      return *this;
    __reset();
    if constexpr (_AllocTraits::propagate_on_container_move_assignment::value)
      alloc_ = std::move(__other.alloc_);
    if (__other.valueless_after_move()) {
      __ptr_ = nullptr;
    } else if (alloc_ == __other.alloc_) {
      __ptr_         = __other.__ptr_;
      __other.__ptr_ = nullptr;
    } else {
      __ptr_ = __make(alloc_, std::move(*__other));
      __other.__reset();
    }
    return *this;
  }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr const _Tp& operator*() const& noexcept { return *__ptr_; }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr _Tp& operator*() & noexcept { return *__ptr_; }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr const _Tp&& operator*() const&& noexcept { return std::move(*__ptr_); }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr _Tp&& operator*() && noexcept { return std::move(*__ptr_); }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr const_pointer operator->() const noexcept { return __ptr_; }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr pointer operator->() noexcept { return __ptr_; }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr bool valueless_after_move() const noexcept { return __ptr_ == nullptr; }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr allocator_type get_allocator() const noexcept { return alloc_; }

  _LIBCPP_HIDE_FROM_ABI constexpr void swap(indirect& __other) noexcept(
      _AllocTraits::propagate_on_container_swap::value || _AllocTraits::is_always_equal::value) {
    using std::swap;
    if constexpr (_AllocTraits::propagate_on_container_swap::value)
      swap(alloc_, __other.alloc_);
    swap(__ptr_, __other.__ptr_);
  }

  _LIBCPP_HIDE_FROM_ABI friend constexpr void swap(indirect& __x, indirect& __y) noexcept(noexcept(__x.swap(__y))) {
    __x.swap(__y);
  }

  template <class _Up, class _AA>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI friend constexpr bool operator==(const indirect& __x, const indirect<_Up, _AA>& __y) {
    if (__x.valueless_after_move() || __y.valueless_after_move())
      return __x.valueless_after_move() == __y.valueless_after_move();
    return *__x == *__y;
  }

  template <class _Up, class _AA>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI friend constexpr auto operator<=>(const indirect& __x, const indirect<_Up, _AA>& __y) {
    if (__x.valueless_after_move() || __y.valueless_after_move())
      return !__x.valueless_after_move() <=> !__y.valueless_after_move();
    return *__x <=> *__y;
  }

  // Precondition: !__x.valueless_after_move(). There is no value-preserving
  // way to compare a value-less indirect against a bare value, so unlike the
  // indirect-vs-indirect overloads above (which can fall back to comparing
  // "is valueless"), this is only well-defined when __x owns a value.
  template <class _Up>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI friend constexpr bool operator==(const indirect& __x, const _Up& __v) {
    return *__x == __v;
  }

  // Precondition: !__x.valueless_after_move(). See operator== above.
  template <class _Up>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI friend constexpr auto operator<=>(const indirect& __x, const _Up& __v) {
    return *__x <=> __v;
  }

private:
  template <class... _Args>
  _LIBCPP_HIDE_FROM_ABI static constexpr pointer __make(_Allocator& __a, _Args&&... __args) {
    pointer __p = _AllocTraits::allocate(__a, 1);
    _AllocTraits::construct(__a, std::addressof(*__p), std::forward<_Args>(__args)...);
    return __p;
  }

  _LIBCPP_HIDE_FROM_ABI constexpr void __reset() {
    if (__ptr_ != nullptr) {
      _AllocTraits::destroy(alloc_, std::addressof(*__ptr_));
      _AllocTraits::deallocate(alloc_, __ptr_, 1);
      __ptr_ = nullptr;
    }
  }

  [[no_unique_address]] _Allocator alloc_{};
  pointer __ptr_ = nullptr;
};

template <class _Value>
indirect(_Value) -> indirect<_Value>;

template <class _Allocator, class _Value>
indirect(allocator_arg_t, _Allocator, _Value)
    -> indirect<_Value, typename allocator_traits<_Allocator>::template rebind_alloc<_Value>>;

template <class _Tp, class _Allocator>
  requires __has_enabled_hash<_Tp>::value
struct hash<indirect<_Tp, _Allocator>> {
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI size_t operator()(const indirect<_Tp, _Allocator>& __i) const
      noexcept(noexcept(hash<remove_const_t<_Tp>>()(*__i))) {
    return __i.valueless_after_move() ? static_cast<size_t>(-1) : hash<remove_const_t<_Tp>>()(*__i);
  }
};

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP___MEMORY_INDIRECT_H
