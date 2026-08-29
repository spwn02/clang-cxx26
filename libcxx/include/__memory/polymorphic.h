// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// std::polymorphic implementation for the Bloomberg libc++ fork.
// P3019R14: indirect and polymorphic: Vocabulary Types for Composite Class Design.
// Adopted into the C++26 working draft at the 2025-02 Hagenberg meeting.
//
// The standard wording specifies polymorphic's copy/destroy behavior only in
// terms of *what* happens ("constructs an owned object of type U, where U is
// the type of the owned object in other"), not *how* -- the mechanism used to
// dynamically dispatch to the owned object's real type is implementation-
// defined. This implementation uses a small manually-managed control block
// (clone/destroy via ordinary virtual functions) rather than a function-
// pointer manager; either is conforming.
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___MEMORY_POLYMORPHIC_H
#define _LIBCPP___MEMORY_POLYMORPHIC_H

#include <__concepts/derived_from.h>
#include <__config>
#include <__memory/allocator.h>
#include <__memory/allocator_arg_t.h>
#include <__memory/allocator_traits.h>
#include <__type_traits/is_array.h>
#include <__type_traits/is_base_of.h>
#include <__type_traits/is_constructible.h>
#include <__type_traits/is_object.h>
#include <__type_traits/is_same.h>
#include <__type_traits/remove_cvref.h>
#include <__utility/forward.h>
#include <__utility/in_place.h>
#include <__utility/move.h>
#include <__utility/swap.h>
#include <initializer_list>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_PUSH_MACROS
#include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 26

template <class _Tp, class _Allocator = allocator<_Tp>>
class polymorphic {
  using _AllocTraits = allocator_traits<_Allocator>;

  static_assert(is_same_v<typename _AllocTraits::value_type, _Tp>,
                "allocator_traits<Allocator>::value_type must be the same type as T");
  static_assert(is_object_v<_Tp> && !is_array_v<_Tp> && !is_same_v<_Tp, in_place_t> && !__is_inplace_type<_Tp>::value &&
                    !is_const_v<_Tp> && !is_volatile_v<_Tp>,
                "T must be a non-array object type that is not in_place_t, a specialization of in_place_type_t, "
                "or cv-qualified");

  struct __control_block {
    _LIBCPP_HIDE_FROM_ABI __control_block()                                          = default;
    _LIBCPP_HIDE_FROM_ABI __control_block(const __control_block&)                    = delete;
    _LIBCPP_HIDE_FROM_ABI virtual ~__control_block()                                 = default;
    _LIBCPP_HIDE_FROM_ABI virtual _Tp* __get() noexcept                              = 0;
    _LIBCPP_HIDE_FROM_ABI virtual __control_block* __clone(_Allocator&) const        = 0;
    // Constructs a new control block whose owned object is move-constructed from
    // this one's, without destroying or otherwise invalidating this one -- used
    // where the standard specifies "constructs ... considering the owned object
    // in other as an rvalue" without also requiring other to become valueless.
    _LIBCPP_HIDE_FROM_ABI virtual __control_block* __move_clone(_Allocator&)         = 0;
    _LIBCPP_HIDE_FROM_ABI virtual void __destroy(_Allocator&)                        = 0;
  };

  template <class _Up>
  struct __control_block_impl final : __control_block {
    _Up __value_;

    template <class... _Args>
    _LIBCPP_HIDE_FROM_ABI explicit __control_block_impl(_Args&&... __args) : __value_(std::forward<_Args>(__args)...) {}

    _LIBCPP_HIDE_FROM_ABI _Tp* __get() noexcept override { return std::addressof(__value_); }

    _LIBCPP_HIDE_FROM_ABI __control_block* __clone(_Allocator& __a) const override {
      using _Rebound = typename _AllocTraits::template rebind_alloc<__control_block_impl>;
      using _RTraits  = allocator_traits<_Rebound>;
      _Rebound __ra(__a);
      auto* __mem = _RTraits::allocate(__ra, 1);
      _RTraits::construct(__ra, __mem, __value_);
      return __mem;
    }

    _LIBCPP_HIDE_FROM_ABI __control_block* __move_clone(_Allocator& __a) override {
      using _Rebound = typename _AllocTraits::template rebind_alloc<__control_block_impl>;
      using _RTraits  = allocator_traits<_Rebound>;
      _Rebound __ra(__a);
      auto* __mem = _RTraits::allocate(__ra, 1);
      _RTraits::construct(__ra, __mem, std::move(__value_));
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

  _LIBCPP_HIDE_FROM_ABI explicit constexpr polymorphic()
    requires is_default_constructible_v<_Allocator>
  {
    static_assert(is_default_constructible_v<_Tp> && is_copy_constructible_v<_Tp>,
                  "T must be default-constructible and copy-constructible");
    cb_ = __make_cb<_Tp>();
  }

  _LIBCPP_HIDE_FROM_ABI explicit constexpr polymorphic(allocator_arg_t, const _Allocator& __a) : alloc_(__a) {
    static_assert(is_default_constructible_v<_Tp> && is_copy_constructible_v<_Tp>,
                  "T must be default-constructible and copy-constructible");
    cb_ = __make_cb<_Tp>();
  }

  _LIBCPP_HIDE_FROM_ABI constexpr polymorphic(const polymorphic& __other)
      : alloc_(_AllocTraits::select_on_container_copy_construction(__other.alloc_)) {
    cb_ = __other.valueless_after_move() ? nullptr : __other.cb_->__clone(alloc_);
  }

  _LIBCPP_HIDE_FROM_ABI constexpr polymorphic(allocator_arg_t, const _Allocator& __a, const polymorphic& __other)
      : alloc_(__a) {
    cb_ = __other.valueless_after_move() ? nullptr : __other.cb_->__clone(alloc_);
  }

  _LIBCPP_HIDE_FROM_ABI constexpr polymorphic(polymorphic&& __other) noexcept
      : alloc_(std::move(__other.alloc_)), cb_(__other.cb_) {
    __other.cb_ = nullptr;
  }

  _LIBCPP_HIDE_FROM_ABI constexpr polymorphic(allocator_arg_t, const _Allocator& __a, polymorphic&& __other) noexcept(
      _AllocTraits::is_always_equal::value)
      : alloc_(__a) {
    if (__other.valueless_after_move()) {
      cb_ = nullptr;
    } else if (alloc_ == __other.alloc_) {
      cb_         = __other.cb_;
      __other.cb_ = nullptr;
    } else {
      cb_ = __other.cb_->__move_clone(alloc_);
    }
  }

  template <class _Up = _Tp>
    requires(!is_same_v<remove_cvref_t<_Up>, polymorphic> && derived_from<remove_cvref_t<_Up>, _Tp> &&
             is_constructible_v<remove_cvref_t<_Up>, _Up> && is_copy_constructible_v<remove_cvref_t<_Up>> &&
             !__is_inplace_type<remove_cvref_t<_Up>>::value && is_default_constructible_v<_Allocator>)
  _LIBCPP_HIDE_FROM_ABI explicit constexpr polymorphic(_Up&& __u) {
    cb_ = __make_cb<remove_cvref_t<_Up>>(std::forward<_Up>(__u));
  }

  template <class _Up = _Tp>
    requires(!is_same_v<remove_cvref_t<_Up>, polymorphic> && derived_from<remove_cvref_t<_Up>, _Tp> &&
             is_constructible_v<remove_cvref_t<_Up>, _Up> && is_copy_constructible_v<remove_cvref_t<_Up>> &&
             !__is_inplace_type<remove_cvref_t<_Up>>::value)
  _LIBCPP_HIDE_FROM_ABI explicit constexpr polymorphic(allocator_arg_t, const _Allocator& __a, _Up&& __u) : alloc_(__a) {
    cb_ = __make_cb<remove_cvref_t<_Up>>(std::forward<_Up>(__u));
  }

  template <class _Up, class... _Ts>
    requires(is_same_v<remove_cvref_t<_Up>, _Up> && derived_from<_Up, _Tp> && is_constructible_v<_Up, _Ts...> &&
             is_copy_constructible_v<_Up> && is_default_constructible_v<_Allocator>)
  _LIBCPP_HIDE_FROM_ABI explicit constexpr polymorphic(in_place_type_t<_Up>, _Ts&&... __ts) {
    cb_ = __make_cb<_Up>(std::forward<_Ts>(__ts)...);
  }

  template <class _Up, class... _Ts>
    requires(is_same_v<remove_cvref_t<_Up>, _Up> && derived_from<_Up, _Tp> && is_constructible_v<_Up, _Ts...> &&
             is_copy_constructible_v<_Up>)
  _LIBCPP_HIDE_FROM_ABI explicit constexpr polymorphic(allocator_arg_t, const _Allocator& __a, in_place_type_t<_Up>, _Ts&&... __ts)
      : alloc_(__a) {
    cb_ = __make_cb<_Up>(std::forward<_Ts>(__ts)...);
  }

  template <class _Up, class _Ip, class... _Us>
    requires(is_same_v<remove_cvref_t<_Up>, _Up> && derived_from<_Up, _Tp> &&
             is_constructible_v<_Up, initializer_list<_Ip>&, _Us...> && is_copy_constructible_v<_Up> &&
             is_default_constructible_v<_Allocator>)
  _LIBCPP_HIDE_FROM_ABI explicit constexpr polymorphic(in_place_type_t<_Up>, initializer_list<_Ip> __il, _Us&&... __us) {
    cb_ = __make_cb<_Up>(__il, std::forward<_Us>(__us)...);
  }

  template <class _Up, class _Ip, class... _Us>
    requires(is_same_v<remove_cvref_t<_Up>, _Up> && derived_from<_Up, _Tp> &&
             is_constructible_v<_Up, initializer_list<_Ip>&, _Us...> && is_copy_constructible_v<_Up>)
  _LIBCPP_HIDE_FROM_ABI explicit constexpr polymorphic(
      allocator_arg_t, const _Allocator& __a, in_place_type_t<_Up>, initializer_list<_Ip> __il, _Us&&... __us)
      : alloc_(__a) {
    cb_ = __make_cb<_Up>(__il, std::forward<_Us>(__us)...);
  }

  _LIBCPP_HIDE_FROM_ABI constexpr ~polymorphic() { __reset(); }

  _LIBCPP_HIDE_FROM_ABI constexpr polymorphic& operator=(const polymorphic& __other) {
    if (std::addressof(__other) == this)
      return *this;
    const bool __needs_updating = _AllocTraits::propagate_on_container_copy_assignment::value;
    __control_block* __new_cb =
        __other.valueless_after_move()
            ? nullptr
            : __other.cb_->__clone(__needs_updating ? const_cast<_Allocator&>(__other.alloc_) : alloc_);
    __reset();
    cb_ = __new_cb;
    if (__needs_updating)
      alloc_ = __other.alloc_;
    return *this;
  }

  _LIBCPP_HIDE_FROM_ABI constexpr polymorphic& operator=(polymorphic&& __other) noexcept(
      _AllocTraits::propagate_on_container_move_assignment::value || _AllocTraits::is_always_equal::value) {
    if (std::addressof(__other) == this)
      return *this;
    const bool __needs_updating = _AllocTraits::propagate_on_container_move_assignment::value;
    if (alloc_ == __other.alloc_) {
      using std::swap;
      swap(cb_, __other.cb_);
      __other.__reset();
    } else {
      __control_block* __new_cb =
          __other.valueless_after_move() ? nullptr : __other.cb_->__clone(__needs_updating ? __other.alloc_ : alloc_);
      __reset();
      cb_ = __new_cb;
      __other.__reset();
    }
    if (__needs_updating)
      alloc_ = std::move(__other.alloc_);
    return *this;
  }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr const _Tp& operator*() const noexcept { return *cb_->__get(); }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr _Tp& operator*() noexcept { return *cb_->__get(); }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr const _Tp* operator->() const noexcept { return cb_->__get(); }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr _Tp* operator->() noexcept { return cb_->__get(); }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr bool valueless_after_move() const noexcept { return cb_ == nullptr; }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI constexpr allocator_type get_allocator() const noexcept { return alloc_; }

  _LIBCPP_HIDE_FROM_ABI constexpr void swap(polymorphic& __other) noexcept(
      _AllocTraits::propagate_on_container_swap::value || _AllocTraits::is_always_equal::value) {
    using std::swap;
    if constexpr (_AllocTraits::propagate_on_container_swap::value)
      swap(alloc_, __other.alloc_);
    swap(cb_, __other.cb_);
  }

  _LIBCPP_HIDE_FROM_ABI friend constexpr void swap(polymorphic& __x, polymorphic& __y) noexcept(noexcept(__x.swap(__y))) {
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
