//===----------------------------------------------------------------------===//
//
// Copyright 2026 Bloomberg Finance L.P.
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
// Upstream bloomberg/clang-p2996 issue #188: 'display_string_of(dealias(R))'
// was not a constant expression for a reflection R of certain alias-template
// specialization types (the exact upstream repro: reflecting
// 'decltype(std::ranges::max_element(r))' for various range types 'r').
// Root cause: 'dealias' (== 'underlying_entity_of') strips alias sugar via a
// hand-rolled loop over specific Type subclasses (TypedefType, UsingType,
// alias TemplateSpecializationType, AutoType, ...). That loop was missing
// DecltypeType -- so when an alias template's underlying type (e.g.
// 'iterator_t<R> = decltype(ranges::begin(declval<R&>()))') desugared one
// step to a DecltypeType node, the loop gave up without going further, even
// though the DecltypeType's own *canonical* type was perfectly ordinary. The
// resulting still-sugared reflection was then handed to the pretty-printer,
// whose 'render_template_argument_list_of' recurses into each template
// argument -- and this particular DecltypeType's template-argument query
// resolved back to an equivalent, still-unresolved reflection every time,
// so the recursion never terminated (confirmed via traced Type* identity:
// the exact same node recurred at every one of >100 levels, eventually
// exhausting the evaluator's call-stack depth budget with an unhelpful
// generic "not a constant expression" diagnostic and no indication of the
// real cause). Fixed by adding DecltypeType to desugarType()'s unconditional
// (non-UnwrapAliases-gated) strip set, alongside the pre-existing AutoType/
// SubstTemplateTypeParmType/ReflectionSpliceType handling -- see
// clang/lib/AST/ExprConstantMeta.cpp's desugarType(). (A related dead-code
// bug in the same loop -- the UsingType branch tested the wrong local, so it
// could never fire -- was fixed alongside this, though it wasn't the cause
// of this particular issue.)

#include <algorithm>
#include <meta>
#include <ranges>
#include <string>

using namespace std::meta;

consteval bool test() {
  std::string s{};

  // The dealiased reflection must be usable in a constant expression at
  // all (the primary symptom: this used to hit the evaluator's constexpr
  // call-depth limit via a non-terminating recursion).
  auto name = display_string_of(dealias(^^decltype(std::ranges::max_element(s))));

  // It must also have actually resolved past the alias sugar down to the
  // real iterator type, not merely avoided crashing.
  return name == "__wrap_iter<char *>";
}
static_assert(test());

consteval bool test_owning_view() {
  std::string s{};
  std::ranges::owning_view ov(std::move(s));

  // A second dealias() round-trip on an already-dealiased reflection must
  // also terminate and be idempotent.
  auto r = ^^decltype(std::ranges::max_element(ov));
  auto once = display_string_of(dealias(r));
  auto twice = display_string_of(dealias(dealias(r)));
  return once == twice && once == "__wrap_iter<char *>";
}
static_assert(test_owning_view());

int main() {
  return 0;
}
