//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

// <memory>

// LWG4148
// Mandates: reference_converts_from_temporary_v<add_lvalue_reference_t<T>, decltype(*declval<pointer>())> is false.

#include <cstddef>
#include <memory>

struct ProxyPointer {
  ProxyPointer(std::nullptr_t = nullptr) {}
  bool operator==(const ProxyPointer&) const = default;
  explicit operator bool() const { return true; }
  int operator*() const { return 0; } // returns a prvalue
};

struct Deleter {
  using pointer = ProxyPointer;
  void operator()(ProxyPointer) const {}
};

void test() {
  std::unique_ptr<const int, Deleter> p;
  // expected-error@*:* {{unique_ptr::operator* would return a reference bound to a temporary}}
  (void)*p;
  // expected-error@*:* 0-1 {{returning reference to local temporary object}}
}
