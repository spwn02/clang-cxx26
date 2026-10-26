//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// The Constraints are in the headers of C++11 and later; the frozen C++03 headers do not have them.
// UNSUPPORTED: c++03

// template<class Rep2, class Period2>
//   constexpr duration(const duration<Rep2, Period2>&);
//
// Constraints: is_convertible_v<const Rep2&, rep> is true.

#include <chrono>
#include <type_traits>

struct ExplicitRep {
  explicit operator int() const { return 1; }
};

static_assert(!std::is_convertible<const ExplicitRep&, int>::value, "");
static_assert(!std::is_constructible<std::chrono::duration<int>, std::chrono::duration<ExplicitRep>>::value, "");
static_assert(std::is_constructible<std::chrono::duration<int>, std::chrono::duration<short>>::value, "");

int main(int, char**) { return 0; }
