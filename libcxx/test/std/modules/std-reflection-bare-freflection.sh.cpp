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

// Regression test for issue #5: libcxx/modules/std/meta.inc guarded its
// exports with a single `#if __has_feature(reflection)`, but <meta> itself
// gates several members (parameters_of, annotations_of, attributes_of, and
// friends) behind the finer-grained `parameter_reflection`,
// `annotation_attributes`, and `attribute_reflection` feature flags. Under
// bare `-freflection` (without also enabling those finer flags, as
// `-freflection-latest` does), those `using` declarations named entities
// that <meta> simply never declared, so `import std;` failed outright with
// ~16 errors -- even though the base reflection facilities that only need
// `-freflection` are otherwise fully usable.
//
// This is a hand-rolled module build (rather than using the usual module
// dependency directive) because that directive's additional-compile-flags
// mechanism doesn't propagate into the std.pcm build step -- the std.pcm
// built that way never has reflection enabled at all, so it can't exercise
// this combination.

// RUN: mkdir %t
// RUN: %{cxx} %{compile_flags} -std=c++26 -freflection \
// RUN:     -Wno-reserved-module-identifier -Wno-reserved-user-defined-literal \
// RUN:     --precompile -o %t/std.pcm -c %{module-dir}/std.cppm
// RUN: %{cxx} %{compile_flags} %{link_flags} -std=c++26 -freflection \
// RUN:     -fmodule-file=std=%t/std.pcm %t/std.pcm \
// RUN:     %s -o %t/std-reflection-bare-freflection.sh.cpp.tsk
// RUN: %{exec} %t/std-reflection-bare-freflection.sh.cpp.tsk

import std;

int main(int, char**) {
  // Exercise only the base reflection facilities that require nothing more
  // than `-freflection` -- deliberately not touching parameters_of/
  // annotations_of/attributes_of and friends, which correctly remain
  // unavailable without their own finer-grained flags.
  constexpr auto r = ^^int;
  static_assert(std::meta::is_type(r));
  static_assert(std::meta::type_of(std::meta::reflect_constant(42)) == r);
  static_assert(std::meta::identifier_of(^^std) == "std");

  return 0;
}
