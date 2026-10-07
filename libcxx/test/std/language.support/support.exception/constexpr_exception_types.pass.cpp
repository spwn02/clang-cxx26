//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: no-exceptions

// P3378R2 constexpr exception types: the special member functions and what() of exception, bad_exception, bad_alloc,
// bad_array_new_length, bad_cast, bad_typeid, bad_optional_access, bad_variant_access and bad_expected_access can be
// used in constant expressions, and optional::value, variant's get and expected::value throw them there.

#include <cassert>
#include <exception>
#include <expected>
#include <new>
#include <optional>
#include <string_view>
#include <type_traits>
#include <typeinfo>
#include <variant>

// what() is an implementation-defined NTBS, only a non-null result can be relied upon
template <class E>
constexpr bool throw_and_catch() {
  try {
    throw E();
  } catch (const std::exception& e) {
    return e.what() != nullptr && std::string_view(e.what()).size() != 0;
  }
  return false;
}

static_assert(throw_and_catch<std::exception>());
static_assert(throw_and_catch<std::bad_exception>());
static_assert(throw_and_catch<std::bad_alloc>());
static_assert(throw_and_catch<std::bad_array_new_length>());
static_assert(throw_and_catch<std::bad_cast>());
static_assert(throw_and_catch<std::bad_typeid>());
static_assert(throw_and_catch<std::bad_optional_access>());
static_assert(throw_and_catch<std::bad_variant_access>());

// copying and assigning
template <class E>
constexpr bool copy_and_assign() {
  E a;
  E b(a);
  b = a;
  return std::string_view(b.what()) == std::string_view(a.what());
}
static_assert(copy_and_assign<std::exception>());
static_assert(copy_and_assign<std::bad_exception>());
static_assert(copy_and_assign<std::bad_alloc>());
static_assert(copy_and_assign<std::bad_array_new_length>());
static_assert(copy_and_assign<std::bad_cast>());
static_assert(copy_and_assign<std::bad_typeid>());
static_assert(copy_and_assign<std::bad_optional_access>());

// the exception thrown by the library
constexpr bool optional_value() {
  try {
    std::optional<int> o;
    (void)o.value();
  } catch (const std::bad_optional_access&) {
    return true;
  }
  return false;
}
static_assert(optional_value());

constexpr bool variant_get() {
  try {
    std::variant<int, long> v;
    (void)std::get<1>(v);
  } catch (const std::bad_variant_access&) {
    return true;
  }
  return false;
}
static_assert(variant_get());

constexpr bool expected_value() {
  try {
    std::expected<int, int> e = std::unexpected(3);
    (void)e.value();
  } catch (const std::bad_expected_access<int>& e) {
    return e.error() == 3 && std::string_view(static_cast<const std::bad_expected_access<void>&>(e).what()) != "";
  }
  return false;
}
static_assert(expected_value());

// the what() strings are the same in constant evaluation and at run time
template <class E>
constexpr const char* what_of() {
  return E().what();
}
constexpr std::string_view bad_alloc_what = what_of<std::bad_alloc>();
constexpr std::string_view bad_cast_what  = what_of<std::bad_cast>();

int main(int, char**) {
  assert(bad_alloc_what == std::bad_alloc().what());
  assert(bad_cast_what == std::bad_cast().what());
  assert(std::string_view(std::bad_optional_access().what()) == "bad_optional_access");
  return 0;
}
