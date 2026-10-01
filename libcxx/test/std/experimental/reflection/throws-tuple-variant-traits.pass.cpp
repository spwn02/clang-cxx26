//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20 || c++23
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

#include <meta>
#include <array>
#include <tuple>
#include <utility>
#include <variant>

struct B {};

consteval bool tuple_size_int() {
  try { (void)(std::meta::tuple_size(^^int)); }
  catch (std::meta::exception&) { return true; }
  return false;
}
static_assert(tuple_size_int());

consteval bool tuple_size_class() {
  try { (void)(std::meta::tuple_size(^^B)); }
  catch (std::meta::exception&) { return true; }
  return false;
}
static_assert(tuple_size_class());

consteval bool tuple_element_int() {
  try { (void)(std::meta::tuple_element(0, ^^int)); }
  catch (std::meta::exception&) { return true; }
  return false;
}
static_assert(tuple_element_int());

consteval bool tuple_element_class() {
  try { (void)(std::meta::tuple_element(0, ^^B)); }
  catch (std::meta::exception&) { return true; }
  return false;
}
static_assert(tuple_element_class());

consteval bool tuple_element_index() {
  try { (void)(std::meta::tuple_element(2, ^^std::tuple<int, int>)); }
  catch (std::meta::exception&) { return true; }
  return false;
}
static_assert(tuple_element_index());

consteval bool variant_size_int() {
  try { (void)(std::meta::variant_size(^^int)); }
  catch (std::meta::exception&) { return true; }
  return false;
}
static_assert(variant_size_int());

consteval bool variant_size_tuple() {
  try { (void)(std::meta::variant_size(^^std::tuple<int>)); }
  catch (std::meta::exception&) { return true; }
  return false;
}
static_assert(variant_size_tuple());

consteval bool variant_alternative_int() {
  try { (void)(std::meta::variant_alternative(0, ^^int)); }
  catch (std::meta::exception&) { return true; }
  return false;
}
static_assert(variant_alternative_int());

consteval bool variant_alternative_index() {
  try { (void)(std::meta::variant_alternative(2, ^^std::variant<int, bool>)); }
  catch (std::meta::exception&) { return true; }
  return false;
}
static_assert(variant_alternative_index());


static_assert(std::meta::tuple_size(^^std::tuple<int, int>) == 2);
static_assert(std::meta::tuple_size(^^std::array<int, 3>) == 3);
static_assert(std::meta::tuple_size(^^std::pair<int, bool>) == 2);
static_assert(std::meta::tuple_element(1, ^^std::tuple<int, bool>) == ^^bool);
static_assert(std::meta::tuple_element(2, ^^std::array<int, 3>) == ^^int);
static_assert(std::meta::tuple_element(0, ^^std::pair<int, bool>) == ^^int);
static_assert(std::meta::variant_size(^^std::variant<int, bool>) == 2);
static_assert(std::meta::variant_alternative(1, ^^std::variant<int, bool>) == ^^bool);

int main() {}
