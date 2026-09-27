//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17

#include <ranges>
#include <utility>

#include "../types.h"

template <class T>
constexpr bool has_rvalue_base() {
  return requires(T&& value) {
    { std::move(value).base() } -> std::same_as<std::ranges::iterator_t<std::conditional_t<
        std::same_as<T, InnerIterForward>, ForwardView, InputView>>>;
  };
}

static_assert(has_rvalue_base<InnerIterForward>());
static_assert(!has_rvalue_base<InnerIterInput>());

int main(int, char**) { return 0; }
