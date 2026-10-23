//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17

// <memory>

// constexpr add_lvalue_reference_t<T> operator*() const noexcept(noexcept(*declval<pointer>()));
//
// LWG4148
// Constraints: *declval<pointer>() is a well-formed expression.

#include <cstddef>
#include <memory>

struct NotDereferenceable {
  NotDereferenceable(std::nullptr_t = nullptr) {}
  bool operator==(const NotDereferenceable&) const = default;
  explicit operator bool() const { return true; }
};

struct Deleter {
  using pointer = NotDereferenceable;
  void operator()(NotDereferenceable) const {}
};

template <class UniquePtr>
concept can_dereference = requires(UniquePtr p) { *p; };

static_assert(can_dereference<std::unique_ptr<int>>);
static_assert(!can_dereference<std::unique_ptr<int, Deleter>>);
