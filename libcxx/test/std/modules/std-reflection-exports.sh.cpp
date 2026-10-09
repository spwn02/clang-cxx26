//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20
// UNSUPPORTED: clang-modules-build
// UNSUPPORTED: gcc

// XFAIL: has-no-cxx-module-support

// Regression test: libcxx/modules/std/meta.inc did not export every name that
// <meta> declares (e.g. has_parent, current_function, is_final,
// subobjects_of, the op_* constants, std::meta::exception, is_string_literal),
// so `import std;` could not see them even though `#include <meta>` could.
//
// std.pcm is built without any warning suppression.
//
// This is a hand-rolled module build for the reason given in
// std-reflection-bare-freflection.sh.cpp: the module-dependency directive does
// not pass the reflection flags on to the std.pcm build.

// RUN: mkdir %t
// RUN: %{cxx} %{compile_flags} -std=c++26 -freflection-latest \
// RUN:     -fentity-proxy-reflection \
// RUN:     -Wno-reserved-module-identifier -Wno-reserved-user-defined-literal \
// RUN:     --precompile -o %t/std.pcm -c %{module-dir}/std.cppm
// RUN: %{cxx} %{compile_flags} %{link_flags} -std=c++26 -freflection-latest \
// RUN:     -fentity-proxy-reflection -Wno-deprecated-declarations \
// RUN:     -fmodule-file=std=%t/std.pcm %t/std.pcm \
// RUN:     %s -o %t/std-reflection-exports.sh.cpp.tsk
// RUN: %{exec} %t/std-reflection-exports.sh.cpp.tsk

import std;

struct S {
  int i;
  void f() {}
};
constexpr int global_object = 1;
[[nodiscard]] int attributed();
struct Base {};
struct Derived final : Base {};
enum class E { a };

consteval bool current_function_works() {
  return std::meta::is_function(std::meta::current_function());
}

consteval bool exception_is_a_std_exception() {
  try {
    throw std::meta::exception(u8"error", ^^int);
  } catch (const std::exception& e) {
    return std::string_view(e.what()) == "error";
  }
  return false;
}

consteval bool synthesized_meta_exception_is_catchable() {
  try {
    (void)std::meta::type_of(^^int);
  } catch (const std::meta::exception&) {
    return true;
  }
  return false;
}

// Type ordering is exported from both <compare> and <meta>.
static_assert(std::type_order_v<int, char> != 0);
static_assert(std::type_order<int, int>::value == std::strong_ordering::equal);
static_assert(std::meta::type_order(^^int, ^^char) == std::type_order_v<int, char>);

// Names <meta> declares in std::meta.
static_assert(std::meta::has_parent(^^S::i));
static_assert(!std::meta::has_parent(^^::));
static_assert(std::meta::is_final(^^Derived));
static_assert(!std::meta::is_final(^^Base));
static_assert(std::meta::is_object(std::meta::reflect_object(global_object)));
static_assert(std::meta::is_value(std::meta::reflect_constant(1)));
static_assert(std::meta::is_structural_type(^^int));
static_assert(std::is_structural_v<int>);
static_assert(std::is_structural<const int>::value);
static_assert(std::is_reflection_v<std::meta::info>);
static_assert(std::is_reflection<const volatile std::meta::info>::value);
static_assert(std::meta::has_c_language_linkage(^^S::i) == false);
static_assert(std::meta::current_namespace() == ^^::);
static_assert(current_function_works());
static_assert(std::meta::subobjects_of(^^S, std::meta::access_context::unchecked()).size() == 1);
static_assert(std::meta::is_trivially_constructible_type(^^int, {}));

// The enumerators of `operators`, and the functions over them.
static_assert(std::meta::op_plus == std::meta::operators::op_plus);
static_assert(std::meta::symbol_of(std::meta::op_plus) == "+");
static_assert(std::meta::u8symbol_of(std::meta::op_plus) == u8"+");

void varargs_fn(int, ...);
static_assert(std::meta::is_vararg_function(^^varargs_fn));

// std::meta::exception, which is-a std::exception.
static_assert(exception_is_a_std_exception());
static_assert(synthesized_meta_exception_is_catchable());
static_assert(std::is_base_of_v<std::exception, std::meta::exception>);

// Names <meta> declares in std.
static_assert(std::is_string_literal("literal"));
static_assert(std::define_static_object(42) != nullptr);

// entity_proxy_reflection
static_assert(std::meta::underlying_entity_of(^^S) == ^^S);

// attribute_reflection
constexpr auto attribute = std::meta::attributes_of(^^attributed)[0];
static_assert(std::meta::is_unscoped_attribute(attribute));
static_assert(!std::meta::is_clang_attribute(attribute));
static_assert(std::meta::has_attribute(^^attributed, attribute));
static_assert(std::meta::has_attribute(
    ^^attributed, attribute, std::meta::attribute_comparison::ignore_argument));
static_assert(std::meta::u8attribute_token_of(attribute) == u8"nodiscard");

int main(int, char**) { return 0; }
