// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___STACKTRACE_BASIC_STACKTRACE_H
#define _LIBCPP___STACKTRACE_BASIC_STACKTRACE_H

#include <__algorithm/equal.h>
#include <__algorithm/lexicographical_compare_three_way.h>
#include <__compare/ordering.h>
#include <__config>
#include <__functional/hash.h>
#include <__iterator/reverse_iterator.h>
#include <__memory/allocator.h>
#include <__memory/allocator_traits.h>
#include <__stacktrace/stacktrace_decls.h>
#include <__stacktrace/stacktrace_entry.h>
#include <__type_traits/is_nothrow_constructible.h>
#include <__utility/swap.h>
#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <limits>
#include <string>
#include <vector>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_PUSH_MACROS
#include <__undef_macros>

_LIBCPP_BEGIN_NAMESPACE_STD

#if _LIBCPP_STD_VER >= 23

// Raw capture: __stacktrace_capture (declared in
// __stacktrace/stacktrace_decls.h) fills in up to __max_depth program
// counters (after skipping __skip innermost frames) using the platform
// unwinder, growing an internal buffer as needed. Kept as a plain,
// non-template, out-of-line function (stacktrace.cpp) so the capture logic
// -- and its dependency on the platform unwind mechanism -- is compiled
// once, not once per Allocator. The result is the raw material
// basic_stacktrace<Allocator>::current() copies into its own allocator-typed
// storage; it carries no allocator of its own since it never escapes this
// translation.

// [stacktrace.basic], class template basic_stacktrace
template <class _Allocator>
class basic_stacktrace {
public:
  using value_type      = stacktrace_entry;
  using const_reference = const value_type&;
  using reference       = value_type&;

private:
  using __storage_type = vector<value_type, _Allocator>;

public:
  using const_iterator         = typename __storage_type::const_iterator;
  using iterator                = const_iterator;
  using reverse_iterator         = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using difference_type         = typename __storage_type::difference_type;
  using size_type                 = typename __storage_type::size_type;
  using allocator_type          = _Allocator;

  // [stacktrace.basic.ctor], creation and assignment
  //
  // Each overload below calls __stacktrace_capture directly -- never through
  // one another -- and is marked _LIBCPP_NOINLINE so it can never be
  // optimized away into its caller. That guarantees a fixed, known call
  // depth between the user's own call site and __stacktrace_capture: exactly
  // one frame for __stacktrace_capture itself (a real, cross-TU function
  // call that can never be inlined regardless of optimization level) plus
  // exactly one frame for whichever current() overload was called. Passing
  // __internal_skip (2) into __stacktrace_capture removes both of those
  // before the user ever sees a single captured frame, so index 0 of the
  // result is always the user's own call site, consistently across every
  // overload and every optimization level -- not just at -O0, where an
  // ordinary (non-noinline) thin wrapper would otherwise leave a
  // variable, implementation-visible number of extra frames depending on
  // which overload delegated to which.
  [[nodiscard]] _LIBCPP_NOINLINE static basic_stacktrace current(allocator_type __alloc = allocator_type()) noexcept {
    constexpr size_t __internal_skip = 2;
    return __from_pcs(
        std::__stacktrace_capture(__internal_skip, static_cast<size_t>(numeric_limits<size_type>::max())),
        std::move(__alloc));
  }
  [[nodiscard]] _LIBCPP_NOINLINE static basic_stacktrace
  current(size_type __skip, allocator_type __alloc = allocator_type()) noexcept {
    constexpr size_t __internal_skip = 2;
    return __from_pcs(
        std::__stacktrace_capture(__internal_skip + static_cast<size_t>(__skip),
                                  static_cast<size_t>(numeric_limits<size_type>::max())),
        std::move(__alloc));
  }
  [[nodiscard]] _LIBCPP_NOINLINE static basic_stacktrace
  current(size_type __skip, size_type __max_depth, allocator_type __alloc = allocator_type()) noexcept {
    constexpr size_t __internal_skip = 2;
    return __from_pcs(
        std::__stacktrace_capture(__internal_skip + static_cast<size_t>(__skip), static_cast<size_t>(__max_depth)),
        std::move(__alloc));
  }

private:
  // Builds a result from already-captured PCs. This runs after
  // __stacktrace_capture has already returned, so its own call depth is
  // irrelevant to the skip-count reasoning above -- it can be inlined
  // freely.
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI static basic_stacktrace
  __from_pcs(vector<uintptr_t> __pcs, allocator_type __alloc) noexcept {
    basic_stacktrace __result(__alloc);
    // current() is specified noexcept: any failure (allocation) degrades to
    // an empty stacktrace rather than throwing/UB. __stacktrace_capture
    // itself is already noexcept and never throws.
#  if _LIBCPP_HAS_EXCEPTIONS
    try {
#  endif
      __result.__frames_.reserve(__pcs.size());
      for (uintptr_t __pc : __pcs)
        __result.__frames_.push_back(value_type(__pc));
#  if _LIBCPP_HAS_EXCEPTIONS
    } catch (...) {
      __result.__frames_.clear();
    }
#  endif
    return __result;
  }

public:

  _LIBCPP_HIDE_FROM_ABI basic_stacktrace() noexcept(is_nothrow_default_constructible_v<allocator_type>) = default;
  _LIBCPP_HIDE_FROM_ABI explicit basic_stacktrace(const allocator_type& __alloc) noexcept : __frames_(__alloc) {}

  _LIBCPP_HIDE_FROM_ABI basic_stacktrace(const basic_stacktrace&)                = default;
  _LIBCPP_HIDE_FROM_ABI basic_stacktrace(basic_stacktrace&&) noexcept            = default;
  _LIBCPP_HIDE_FROM_ABI basic_stacktrace(const basic_stacktrace& __other, const allocator_type& __alloc)
      : __frames_(__other.__frames_, __alloc) {}
  _LIBCPP_HIDE_FROM_ABI basic_stacktrace(basic_stacktrace&& __other, const allocator_type& __alloc)
      : __frames_(std::move(__other.__frames_), __alloc) {}
  _LIBCPP_HIDE_FROM_ABI basic_stacktrace& operator=(const basic_stacktrace&) = default;
  _LIBCPP_HIDE_FROM_ABI basic_stacktrace& operator=(basic_stacktrace&&) noexcept(
      allocator_traits<_Allocator>::propagate_on_container_move_assignment::value ||
      allocator_traits<_Allocator>::is_always_equal::value) = default;

  _LIBCPP_HIDE_FROM_ABI ~basic_stacktrace() = default;

  // [stacktrace.basic.obs], observers
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI allocator_type get_allocator() const noexcept { return __frames_.get_allocator(); }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI const_iterator begin() const noexcept { return __frames_.begin(); }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI const_iterator end() const noexcept { return __frames_.end(); }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI const_reverse_iterator rbegin() const noexcept {
    return const_reverse_iterator(end());
  }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI const_reverse_iterator rend() const noexcept {
    return const_reverse_iterator(begin());
  }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI const_iterator cbegin() const noexcept { return begin(); }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI const_iterator cend() const noexcept { return end(); }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI const_reverse_iterator crbegin() const noexcept { return rbegin(); }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI const_reverse_iterator crend() const noexcept { return rend(); }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI bool empty() const noexcept { return __frames_.empty(); }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI size_type size() const noexcept { return __frames_.size(); }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI size_type max_size() const noexcept { return __frames_.max_size(); }

  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI const_reference operator[](size_type __i) const { return __frames_[__i]; }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI const_reference at(size_type __i) const { return __frames_.at(__i); }

  // [stacktrace.basic.cmp], comparisons
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI friend bool
  operator==(const basic_stacktrace& __x, const basic_stacktrace& __y) noexcept {
    return std::equal(__x.begin(), __x.end(), __y.begin(), __y.end());
  }
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI friend strong_ordering
  operator<=>(const basic_stacktrace& __x, const basic_stacktrace& __y) noexcept {
    return std::lexicographical_compare_three_way(
        __x.begin(), __x.end(), __y.begin(), __y.end(), [](const value_type& __a, const value_type& __b) {
          return __a.native_handle() <=> __b.native_handle();
        });
  }

  // [stacktrace.basic.mod], modifiers
  _LIBCPP_HIDE_FROM_ABI void swap(basic_stacktrace& __other) noexcept(
      allocator_traits<_Allocator>::propagate_on_container_swap::value ||
      allocator_traits<_Allocator>::is_always_equal::value) {
    __frames_.swap(__other.__frames_);
  }

private:
  __storage_type __frames_;
};

template <class _Allocator>
_LIBCPP_HIDE_FROM_ABI void swap(basic_stacktrace<_Allocator>& __x, basic_stacktrace<_Allocator>& __y) noexcept(
    noexcept(__x.swap(__y))) {
  __x.swap(__y);
}

// [stacktrace.basic.nonmem], non-member functions
using stacktrace = basic_stacktrace<allocator<stacktrace_entry>>;

template <class _Allocator>
[[nodiscard]] _LIBCPP_HIDE_FROM_ABI string to_string(const basic_stacktrace<_Allocator>& __st) {
  string __result;
  for (size_t __i = 0; __i < __st.size(); ++__i) {
    if (__i != 0)
      __result += '\n';
    __result += std::to_string(__i);
    __result += "# ";
    __result += std::to_string(__st[__i]);
  }
  return __result;
}

template <class _CharT, class _Traits, class _Allocator>
_LIBCPP_HIDE_FROM_ABI basic_ostream<_CharT, _Traits>&
operator<<(basic_ostream<_CharT, _Traits>& __os, const basic_stacktrace<_Allocator>& __st) {
  return __os << std::to_string(__st);
}

template <class _Allocator>
struct hash<basic_stacktrace<_Allocator>> {
  [[nodiscard]] _LIBCPP_HIDE_FROM_ABI size_t operator()(const basic_stacktrace<_Allocator>& __st) const noexcept {
    // Unspecified (Hash requirements only demand equal objects hash equal);
    // a simple order-sensitive combine, matching how libc++ combines
    // sub-hashes elsewhere (see hash<tuple<...>>).
    size_t __seed = __st.size();
    for (const auto& __f : __st)
      __seed ^= hash<stacktrace_entry>()(__f) + 0x9e3779b9 + (__seed << 6) + (__seed >> 2);
    return __seed;
  }
};

#endif // _LIBCPP_STD_VER >= 23

_LIBCPP_END_NAMESPACE_STD

_LIBCPP_POP_MACROS

#endif // _LIBCPP___STACKTRACE_BASIC_STACKTRACE_H
