// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// std::indirect implementation for the Bloomberg libc++ fork.
// P3019R14: indirect and polymorphic: Vocabulary Types for Composite Class Design.
// Adopted into the C++26 working draft at the 2025-02 Hagenberg meeting.
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___MEMORY_INDIRECT_H
#define _LIBCPP___MEMORY_INDIRECT_H

#include <__config>
#include <__functional/hash.h>
#include <__memory/allocator.h>
#include <__memory/allocator_traits.h>
#include <__type_traits/is_array.h>
#include <__type_traits/is_assignable.h>
#include <__type_traits/is_constructible.h>
#include <__type_traits/is_object.h>
#include <__type_traits/is_same.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/forward.h>
#include <__utility/in_place.h>
#include <__utility/move.h>
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

  static_assert(is_same_v<typename _AllocTraits::value_type, _Tp>,
                "allocator_traits<Allocator>::value_type must be the same type as T");
  static_assert(is_object_v<_Tp> && !is_array_v<_Tp> && !is_same_v<_Tp, in_place_t> && !__is_inplace_type<_Tp>::value &&
                    !is_const_v<_Tp> && !is_volatile_v<_Tp>,
                "T must be a non-array object type that is not in_place_t, a specialization of in_place_type_t, "
                "or cv-qualified");

public:
  using value_type     = _Tp;
  using allocator_type = _Allocator;
  using pointer         = typename _AllocTraits::pointer;
  using const_pointer   = typename _AllocTraits::const_pointer;

  _LIBCPP_HIDE_FROM_ABI explicit constexpr indirect()
    requires is_default_constructible_v<_Allocator>
  {
    static_assert(is_default_constructible_v<_Tp>, "T must be default-constructible");
    __ptr_ = __make(alloc_);
  }

  _LIBCPP_HIDE_FROM_ABI explicit constexpr indirect(allocator_arg_t, const _Allocator& __a) : alloc_(__a) {
    static_assert(is_default_constructible_v<_Tp>, "T must be default-constructible");
    __ptr_ = __make(alloc_);
  }

  _LIBCPP_HIDE_FROM_ABI constexpr indirect(const indirect& __other)
      : alloc_(_AllocTraits::select_on_container_copy_construction(__other.alloc_)) {
    static_assert(is_copy_constructible_v<_Tp>, "T must be copy-constructible");
    __ptr_ = __other.valueless_after_move() ? nullptr : __make(alloc_, *__other);
  }

  _LIBCPP_HIDE_FROM_ABI constexpr indirect(allocator_arg_t, const _Allocator& __a, const indirect& __other)
      : alloc_(__a) {
    static_assert(is_copy_constructible_v<_Tp>, "T must be copy-constructible");
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
      __other.__reset();
    }
  }

  template <class _Up = _Tp>
    requires(!is_same_v<remove_cvref_t<_Up>, indirect> && !is_same_v<remove_cvref_t<_Up>, in_place_t> &&
             is_constructible_v<_Tp, _Up> && is_default_constructible_v<_Allocator>)
  _LIBCPP_HIDE_FROM_ABI explicit constexpr indirect(_Up&& __u) {
    __ptr_ = __make(alloc_, std::forward<_Up>(__u));
  }

  template <class _Up = _Tp>
    requires(!is_same_v<remove_cvref_t<_Up>, indirect> && !is_same_v<remove_cvref_t<_Up>, in_place_t> &&
             is_constructible_v<_Tp, _Up>)
  _LIBCPP_HIDE_FROM_ABI explicit constexpr indirect(allocator_arg_t, const _Allocator& __a, _Up&& __u) : alloc_(__a) {
    __ptr_ = __make(alloc_, std::forward<_Up>(__u));
  }

  template <class... _Us>
    requires(is_constructible_v<_Tp, _Us...> && is_default_constructible_v<_Allocator>)
  _LIBCPP_HIDE_FROM_ABI explicit constexpr indirect(in_place_t, _Us&&... __us) {
    __ptr_ = __make(alloc_, std::forward<_Us>(__us)...);
  }

  template <class... _Us>
    requires is_constructible_v<_Tp, _Us...>
  _LIBCPP_HIDE_FROM_ABI explicit constexpr indirect(allocator_arg_t, const _Allocator& __a, in_place_t, _Us&&... __us)
      : alloc_(__a) {
    __ptr_ = __make(alloc_, std::forward<_Us>(__us)...);
  }

  template <class _Ip, class... _Us>
    requires(is_constructible_v<_Tp, initializer_list<_Ip>&, _Us...> && is_default_constructible_v<_Allocator>)
  _LIBCPP_HIDE_FROM_ABI explicit constexpr indirect(in_place_t, initializer_list<_Ip> __il, _Us&&... __us) {
    __ptr_ = __make(alloc_, __il, std::forward<_Us>(__us)...);
  }

  template <class _Ip, class... _Us>
    requires is_constructible_v<_Tp, initializer_list<_Ip>&, _Us...>
  _LIBCPP_HIDE_FROM_ABI explicit constexpr indirect(
      allocator_arg_t, const _Allocator& __a, in_place_t, initializer_list<_Ip> __il, _Us&&... __us)
      : alloc_(__a) {
    __ptr_ = __make(alloc_, __il, std::forward<_Us>(__us)...);
  }

  _LIBCPP_HIDE_FROM_ABI constexpr ~indirect() { __reset(); }

  _LIBCPP_HIDE_FROM_ABI constexpr indirect& operator=(const indirect& __other) {
    static_assert(is_copy_assignable_v<_Tp> && is_copy_constructible_v<_Tp>,
                  "T must be copy-assignable and copy-constructible");
    if (std::addressof(__other) == this)
      return *this;
    const bool __needs_updating = _AllocTraits::propagate_on_container_copy_assignment::value;
    if (__other.valueless_after_move()) {
      __reset();
    } else if (alloc_ == __other.alloc_ && !valueless_after_move()) {
      **this = *__other;
    } else {
      _Allocator& __src_alloc = __needs_updating ? const_cast<_Allocator&>(__other.alloc_) : alloc_;
      pointer __new_ptr        = __make(__src_alloc, *__other);
      __reset();
      __ptr_ = __new_ptr;
    }
    if (__needs_updating)
      alloc_ = __other.alloc_;
    return *this;
  }

  // The adopted wording's Mandates clause for this overload literally reads
  // "is_copy_constructible_t<T>" (not _v<T>, and not is_move_constructible),
  // which looks like an editorial slip -- this function never copies T, only
  // moves or swaps it. We deliberately do not enforce that as a static_assert:
  // doing so would reject valid, useful instantiations like
  // indirect<unique_ptr<int>> that are copy-constructible-in-neither-sense
  // but perfectly fine to move-assign.
  _LIBCPP_HIDE_FROM_ABI constexpr indirect& operator=(indirect&& __other) noexcept(
      _AllocTraits::propagate_on_container_move_assignment::value || _AllocTraits::is_always_equal::value) {
    if (std::addressof(__other) == this)
      return *this;
    const bool __needs_updating = _AllocTraits::propagate_on_container_move_assignment::value;
    if (__other.valueless_after_move()) {
      __reset();
    } else if (alloc_ == __other.alloc_) {
      using std::swap;
      swap(__ptr_, __other.__ptr_);
      __other.__reset();
    } else {
      _Allocator& __src_alloc = __needs_updating ? __other.alloc_ : alloc_;
      pointer __new_ptr        = __make(__src_alloc, std::move(*__other));
      __reset();
      __ptr_ = __new_ptr;
      __other.__reset();
    }
    if (__needs_updating)
      alloc_ = std::move(__other.alloc_);
    return *this;
  }

  template <class _Up = _Tp>
    requires(!is_same_v<remove_cvref_t<_Up>, indirect> && is_constructible_v<_Tp, _Up> && is_assignable_v<_Tp&, _Up>)
  _LIBCPP_HIDE_FROM_ABI constexpr indirect& operator=(_Up&& __u) {
    if (valueless_after_move())
      __ptr_ = __make(alloc_, std::forward<_Up>(__u));
    else
      **this = std::forward<_Up>(__u);
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
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI friend constexpr bool
  operator==(const indirect& __x, const indirect<_Up, _AA>& __y) noexcept(noexcept(*__x == *__y)) {
    if (__x.valueless_after_move() || __y.valueless_after_move())
      return __x.valueless_after_move() == __y.valueless_after_move();
    return *__x == *__y;
  }

  template <class _Up, class _AA>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI friend constexpr auto
  operator<=>(const indirect& __x, const indirect<_Up, _AA>& __y) {
    if (__x.valueless_after_move() || __y.valueless_after_move())
      return !__x.valueless_after_move() <=> !__y.valueless_after_move();
    return std::__synth_three_way(*__x, *__y);
  }

  template <class _Up>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI friend constexpr bool operator==(const indirect& __x, const _Up& __v) noexcept(
      noexcept(*__x == __v)) {
    if (__x.valueless_after_move())
      return false;
    return *__x == __v;
  }

  template <class _Up>
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI friend constexpr auto operator<=>(const indirect& __x, const _Up& __v) {
    if (__x.valueless_after_move())
      return strong_ordering::less;
    return std::__synth_three_way(*__x, __v);
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
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI size_t operator()(const indirect<_Tp, _Allocator>& __i) const {
    return __i.valueless_after_move() ? static_cast<size_t>(-1) : hash<remove_const_t<_Tp>>()(*__i);
  }
};

#endif // _LIBCPP_STD_VER >= 26

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP___MEMORY_INDIRECT_H
