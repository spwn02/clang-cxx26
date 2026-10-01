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

using info = std::meta::info;
constexpr auto parameters_origin = ^^std::meta::parameters_of;
constexpr auto variable_origin = ^^std::meta::variable_of;
namespace ns {}
template <class> struct tmpl {};
struct S { int member; void fn(int); };
int object;
void fn(int);
template <class T> void tfn(T);
constexpr auto lambda = [](int) {};
using Closure = decltype(lambda);

consteval bool parameters_throw(info r) {
  try {
    (void)std::meta::parameters_of(r);
  } catch (const std::meta::exception& e) {
    return e.from() == parameters_origin &&
           std::string_view(e.what()) == "invalid reflection operand" &&
           e.where().line() != 0;
  }
  return false;
}

consteval bool variable_throws(info r) {
  try {
    (void)std::meta::variable_of(r);
  } catch (const std::meta::exception& e) {
    return e.from() == variable_origin &&
           !std::string_view(e.what()).empty() &&
           e.where().line() != 0;
  }
  return false;
}

static_assert(parameters_throw(info{}));
static_assert(parameters_throw(^^::));
static_assert(parameters_throw(^^ns));
static_assert(parameters_throw(^^tmpl));
static_assert(parameters_throw(^^tfn));
static_assert(parameters_throw(std::meta::reflect_constant(1)));
static_assert(parameters_throw(^^object));
static_assert(parameters_throw(std::meta::reflect_object(object)));
static_assert(parameters_throw(^^S::member));
static_assert(parameters_throw(std::meta::parameters_of(^^fn)[0]));
static_assert(parameters_throw(^^int));
static_assert(parameters_throw(^^S));

static_assert(variable_throws(info{}));
static_assert(variable_throws(^^::));
static_assert(variable_throws(^^ns));
static_assert(variable_throws(^^tmpl));
static_assert(variable_throws(^^tfn));
static_assert(variable_throws(std::meta::reflect_constant(1)));
static_assert(variable_throws(^^object));
static_assert(variable_throws(std::meta::reflect_object(object)));
static_assert(variable_throws(^^S::member));
static_assert(variable_throws(std::meta::parameters_of(^^fn)[0]));
static_assert(variable_throws(^^int));
static_assert(variable_throws(^^S));
static_assert(variable_throws(^^fn));
static_assert(variable_throws(^^tfn<int>));
static_assert(variable_throws(^^Closure::operator()));
static_assert(variable_throws(^^S::fn));
static_assert(variable_throws(std::meta::type_of(^^fn)));

consteval bool valid_parameters(info r) {
  try {
    auto params = std::meta::parameters_of(r);
    return params.size() == 1 && std::meta::is_function_parameter(params[0]) &&
           std::meta::type_of(params[0]) == ^^int;
  } catch (const std::meta::exception&) {
    return false;
  }
}
static_assert(valid_parameters(^^fn));
static_assert(valid_parameters(^^tfn<int>));
static_assert(valid_parameters(^^Closure::operator()));
static_assert(valid_parameters(^^S::fn));
consteval bool valid_function_type() {
  try {
    return std::meta::parameters_of(std::meta::type_of(^^fn)) == std::vector{^^int};
  } catch (const std::meta::exception&) {
    return false;
  }
}
static_assert(valid_function_type());

consteval bool inside([[maybe_unused]] int c) {
  try {
    return std::meta::variable_of(std::meta::parameters_of(^^inside)[0]) == ^^c;
  } catch (const std::meta::exception&) {
    return false;
  }
}
static_assert(inside(0));

consteval bool nested([[maybe_unused]] int c) {
  return []() consteval {
    try {
      (void)std::meta::variable_of(std::meta::parameters_of(^^nested)[0]);
    } catch (const std::meta::exception& e) {
      return e.from() == variable_origin && e.where().line() != 0 &&
             std::string_view(e.what()) == "invalid reflection operand";
    }
    return false;
  }();
}
static_assert(nested(0));


// Helpers preserve the evaluation context of the static_assert in g.
consteval bool variable_throws_twice(info r) {
  return variable_throws(r);
}
void g([[maybe_unused]] int c, [[maybe_unused]] int d) {
  constexpr auto p = std::meta::parameters_of(^^g)[0];
  static_assert(!variable_throws(p));
  static_assert(!variable_throws_twice(p));
  static_assert([](info r) consteval { return !variable_throws(r); }(p));
  static_assert(std::meta::variable_of(p) == ^^c);
  static_assert([]() consteval { return !variable_throws_twice(p); }());
}
static_assert(variable_throws(std::meta::parameters_of(^^g)[0]));
static_assert(variable_throws_twice(std::meta::parameters_of(^^g)[0]));

int main() {}
