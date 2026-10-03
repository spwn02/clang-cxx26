//===----------------------------------------------------------------------===//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20 || c++23
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest -fattribute-reflection

#include <meta>
using namespace std::meta;

// P4033R1 [meta.reflection.define.enum] declares attributes before annotations.
static_assert(identifier_of(nonstatic_data_members_of(^^enumerator_options, access_context::current())[2])
              == "attributes");
static_assert(identifier_of(nonstatic_data_members_of(^^enumerator_options, access_context::current())[3])
              == "annotations");

// P4033R1 enumerator_spec Throws: each annotation has non-array object type
// and constant_of(r) does not exit via an exception; each attribute is_attribute.
consteval bool rejects(enumerator_options options) {
  try {
    enumerator_spec(options);
  } catch (const exception& e) {
    return e.from() == ^^enumerator_spec;
  }
  return false;
}

constexpr int array[] = {1, 2};
int nonconstant;
void function();
struct S { int member; };
static_assert(rejects({.name="A", .annotations={^^int}}));
static_assert(rejects({.name="A", .annotations={info{}}}));
static_assert(rejects({.name="A", .annotations={^^array}}));
static_assert(rejects({.name="A", .annotations={^^nonconstant}}));
static_assert(rejects({.name="A", .annotations={^^function}}));
static_assert(rejects({.name="A", .annotations={^^S::member}}));
static_assert(rejects({.name="A", .attributes={^^int}}));
static_assert(rejects({.name="A", .attributes={info{}}}));
static_assert(rejects({.name="A", .attributes={^^[[maybe_unused]], ^^int}}));

// P4033R1 enumerator_spec Returns: AN contains constant_of(r), including
// reflections of constant variables rather than only reflected constants.
constexpr int annotation = 42;
enum class Annotated;
consteval {
  define_enum(^^Annotated, {enumerator_spec({.name="A", .attributes={^^[[maybe_unused]]},
                                           .annotations={^^annotation}})});
}
static_assert(annotations_of(^^Annotated::A).size() == 1);
static_assert(extract<int>(annotations_of(^^Annotated::A)[0]) == 42);
static_assert(has_attribute(^^Annotated::A, ^^[[maybe_unused]]));

int main(int, char**) {}
