//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

#include <meta>
#include <string_view>

struct complete { int value; };
void named_parameter(int value);
void unnamed_parameter(int);

template <class T> struct substitution_trait { using type = void; };
template <class T>
using substitution_probe = typename substitution_trait<T &>::type;

consteval bool catches_members_error() {
  try {
    (void)std::meta::members_of(
        ^^int, std::meta::access_context::unchecked());
    return false;
  } catch (const std::meta::exception& e) {
    return e.from() != std::meta::info{} &&
           e.u8what() == u8"invalid reflection operand" &&
           std::string_view(e.what()) == "invalid reflection operand" &&
           e.where().line() != 0;
  }
}

consteval bool inner() { return catches_members_error(); }
consteval bool outer() { return inner(); }

consteval bool catches_representative_failures() {
  bool type = false;
  bool parent = false;
  bool bases = false;
  bool static_members = false;
  bool nonstatic_members = false;
  bool identifier = false;
  bool u8identifier = false;
  bool return_type = false;
  bool parameters = false;
  bool annotations = false;
  try { (void)std::meta::type_of(^^std); }
  catch (const std::meta::exception& e) {
    type = e.from() == ^^std::meta::type_of;
  }
  try { (void)std::meta::parent_of(^^int); }
  catch (const std::meta::exception& e) {
    parent = e.from() == ^^std::meta::parent_of;
  }
  try {
    (void)std::meta::bases_of(
        ^^int, std::meta::access_context::unchecked());
  } catch (const std::meta::exception& e) {
    bases = e.from() != std::meta::info{};
  }
  try {
    (void)std::meta::static_data_members_of(
        ^^int, std::meta::access_context::unchecked());
  } catch (const std::meta::exception& e) {
    static_members = e.from() != std::meta::info{};
  }
  try {
    (void)std::meta::nonstatic_data_members_of(
        ^^int, std::meta::access_context::unchecked());
  } catch (const std::meta::exception& e) {
    nonstatic_members = e.from() != std::meta::info{};
  }
  try { (void)std::meta::identifier_of(^^int); }
  catch (const std::meta::exception& e) {
    identifier = e.from() == ^^std::meta::identifier_of;
  }
  try { (void)std::meta::u8identifier_of(^^int); }
  catch (const std::meta::exception& e) {
    u8identifier = e.from() == ^^std::meta::u8identifier_of;
  }
  try { (void)std::meta::return_type_of(^^int); }
  catch (const std::meta::exception& e) {
    return_type = e.from() == ^^std::meta::return_type_of;
  }
  try { (void)std::meta::parameters_of(^^int); }
  catch (const std::meta::exception& e) {
    parameters = e.from() == ^^std::meta::parameters_of;
  }
  try { (void)std::meta::annotations_of(std::meta::reflect_constant(1)); }
  catch (const std::meta::exception& e) {
    annotations = e.from() != std::meta::info{};
  }
  return type && parent && bases && static_members && nonstatic_members &&
         identifier && u8identifier && return_type && parameters &&
         annotations;
}

static_assert(catches_members_error());
static_assert(outer());
static_assert(catches_representative_failures());
static_assert(std::meta::members_of(
                  ^^complete, std::meta::access_context::unchecked()).size() >=
              1);
static_assert(std::meta::type_of(
                  std::meta::parameters_of(^^named_parameter)[0]) == ^^int);
static_assert(std::meta::identifier_of(
                  std::meta::parameters_of(^^named_parameter)[0]) == "value");
static_assert(std::meta::type_of(
                  std::meta::parameters_of(^^unnamed_parameter)[0]) == ^^int);

consteval bool catches_unnamed_parameter_identifier() {
  try {
    (void)std::meta::identifier_of(
        std::meta::parameters_of(^^unnamed_parameter)[0]);
  } catch (const std::meta::exception& e) {
    return e.from() == ^^std::meta::identifier_of;
  }
  return false;
}

static_assert(catches_unnamed_parameter_identifier());

consteval bool catches_ast_validation_failures() {
  bool object = false;
  bool constant = false;
  bool extract = false;
  bool reflect = false;
  try { (void)std::meta::object_of(^^int); }
  catch (const std::meta::exception& e) {
    object = e.from() == ^^std::meta::object_of;
  }
  try { (void)std::meta::constant_of(^^int); }
  catch (const std::meta::exception& e) {
    constant = e.from() == ^^std::meta::constant_of;
  }
  try { (void)std::meta::extract<int>(^^int); }
  catch (const std::meta::exception& e) {
    extract = e.from() == ^^std::meta::extract;
  }
  try { (void)std::meta::reflect_constant((const char *)"fails"); }
  catch (const std::meta::exception& e) {
    reflect = e.from() != std::meta::info{};
  }
  return object && constant && extract && reflect;
}

static_assert(catches_ast_validation_failures());

consteval bool nested_substitution_probe() {
  try {
    return !std::meta::can_substitute(^^substitution_probe, {^^void});
  } catch (...) {
    return false;
  }
}

static_assert(nested_substitution_probe());
static_assert(std::meta::substitute(^^substitution_probe, {^^int}) !=
              std::meta::info{});

consteval bool catches_invalid_substitution() {
  try {
    (void)std::meta::substitute(^^substitution_probe, {^^void});
  } catch (const std::meta::exception& e) {
    return e.from() != std::meta::info{};
  }
  return false;
}

static_assert(catches_invalid_substitution());

int main(int, char**) { return 0; }
