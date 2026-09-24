//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

// <experimental/reflection>
//
// Deducing the template arguments of a class template from the constructors it
// inherits (C++23 [over.match.class.deduct]) declares an alias template and a
// class template in the namespace of the class template. Those are
// implementation details and must not show up in members_of.

#include <meta>

#include <string_view>

namespace N {
template <class T>
struct B {
  B(T) {}
};
template <class T>
struct C : B<T> {
  using B<T>::B;
};
} // namespace N

constexpr auto unchecked = std::meta::access_context::unchecked();

consteval std::size_t helper_templates() {
  std::size_t count = 0;
  for (auto m : members_of(^^N, unchecked))
    if (has_identifier(m) && identifier_of(m).starts_with("__ctad_"))
      ++count;
  return count;
}

N::C c(1); // declares the deduction guides inherited from B
static_assert(__is_same(decltype(c), N::C<int>));

static_assert(helper_templates() == 0);

int main() { return 0; }
