//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// ADDITIONAL_COMPILE_FLAGS: -ffreestanding -fno-exceptions
//===----------------------------------------------------------------------===//

#include <variant>
#include <utility>
#include <version>

static_assert(__cpp_lib_freestanding_variant == 202311L);

template <class V>
concept has_index_get = requires(V&& value) { std::get<0>(std::forward<V>(value)); };
template <class V>
concept has_type_get = requires(V&& value) { std::get<int>(std::forward<V>(value)); };

using variant_type = std::variant<int, long>;
static_assert(!has_index_get<variant_type&> && !has_type_get<variant_type&>);
static_assert(!has_index_get<const variant_type&> && !has_type_get<const variant_type&>);
static_assert(!has_index_get<variant_type> && !has_type_get<variant_type>);
static_assert(!has_index_get<const variant_type> && !has_type_get<const variant_type>);

constexpr bool test_freestanding_variant() {
  std::variant<int, long> value(42);
  if (int* active = std::get_if<0>(&value))
    *active = 7;
  if (std::visit([](auto active) { return static_cast<long>(active); }, value) != 7)
    return false;
  if (value.visit([](auto active) { return active == 7; }) != true)
    return false;
  value.emplace<long>(9);
  return std::holds_alternative<long>(value) && *std::get_if<long>(&value) == 9;
}

static_assert(test_freestanding_variant());
