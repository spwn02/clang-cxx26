//===----------------------------------------------------------------------===//
//
// Copyright 2024 Bloomberg Finance L.P.
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
// [reflection]
//
// Regression test: building a reflection from a 'Decl *' must yield
// ReflectionKind::Parameter for a function parameter.
//
// 'makeReflection' in ExprConstantMeta.cpp (the path taken by 'parameters_of')
// maps a ParmVarDecl to ReflectionKind::Parameter, but the parallel mapping in
// 'Sema::BuildCXXReflectExpr(SourceLocation, SourceLocation, Decl *)' handled
// only namespaces and entity proxies, so a parameter fell through to
// ReflectionKind::Declaration. Two symptoms followed:
//
//   1. A source-level '^^param' reflected as a declaration, so
//      'is_function_parameter' was false.
//
//   2. TreeTransform::TransformCXXReflectExpr rebuilds a Parameter reflection
//      through that same overload. Synthesizing an expansion statement body
//      rebuilds the body's expressions, so a 'std::meta::info' template
//      argument naming a parameter was silently rewritten from Parameter to
//      Declaration *inside* the body. It then compared unequal to the
//      reflection 'parameters_of' produced for that same parameter -- while
//      the identical comparison just outside the 'template for' compared
//      equal.
//
// Field shape: a 'template for' over cached parameter metadata, matching each
// element against a parameter passed as a template argument, silently matched
// nothing and every parameter looked unannotated.

#include <experimental/meta>
#include <vector>

int fn(int a, double b);

constexpr auto params = std::define_static_array(std::meta::parameters_of(^^fn));

// Symptom 1: a source-level '^^param' is a reflection of a parameter.
consteval auto directlyReflected([[maybe_unused]] int a) -> bool {
  return std::meta::is_function_parameter(^^a);
}
static_assert(directlyReflected(0));

// 'parameters_of' agrees, outside of any expansion statement.
static_assert(std::meta::is_function_parameter(params[0]));
static_assert(std::meta::is_function_parameter(params[1]));

// Symptom 2: the reflection kind survives expansion statement synthesis.
//
// The range being expanded is deliberately unrelated to the reflection under
// test: it is entering the body at all, not the contents of the range, that
// used to rewrite the reflection.
constexpr int unrelated[] = {1, 2, 3};

template <std::meta::info Parameter>
consteval auto stillAParameterInside() -> bool {
  bool result = true;

  template for (constexpr int n : unrelated) {
    result = result && std::meta::is_function_parameter(Parameter);
  }

  return result;
}
static_assert(stillAParameterInside<params[0]>());

// The same comparison must hold inside and outside the body.
template <std::meta::info Parameter>
consteval auto comparesEqualBothSides() -> bool {
  const bool outside = (Parameter == params[0]);
  bool inside = false;

  template for (constexpr int n : unrelated) {
    inside = (Parameter == params[0]);
  }

  return outside && inside;
}
static_assert(comparesEqualBothSides<params[0]>());

// The shape this actually broke in: match cached parameter metadata against a
// parameter named by a template argument.
struct Property {
  std::meta::info parameter;
  bool legacy;
};

template <std::meta::info Function>
consteval auto makeProperties() {
  std::vector<Property> result;

  template for (constexpr std::meta::info parameter :
                std::define_static_array(std::meta::parameters_of(Function))) {
    result.push_back({parameter, false});
  }

  return std::define_static_array(result);
}

template <std::meta::info Function>
struct Metadata {
  static constexpr auto properties = makeProperties<Function>();
};

template <std::meta::info Function, std::meta::info Parameter>
consteval auto countMatching() -> int {
  int result{};

  template for (constexpr Property property : Metadata<Function>::properties) {
    if constexpr (property.parameter == Parameter && !property.legacy)
      ++result;
  }

  return result;
}

static_assert(Metadata<^^fn>::properties.size() == 2);
static_assert(countMatching<^^fn, params[0]>() == 1);
static_assert(countMatching<^^fn, params[1]>() == 1);

// Reflections of other entities keep the kinds they already had.
int gvar;

static_assert(std::meta::is_variable(^^gvar));
static_assert(!std::meta::is_function_parameter(^^gvar));
static_assert(std::meta::is_function(^^fn));
static_assert(std::meta::is_type(^^int));

int main() { return 0; }
