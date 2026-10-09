//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20
// ADDITIONAL_COMPILE_FLAGS: -freflection
// ADDITIONAL_COMPILE_FLAGS: -fannotation-attributes

// <meta>
//
// [meta.reflection.annotation]: annotations are read-only in C++26. The only
// functions are is_annotation, annotations_of(info) and
// annotations_of_with_type(info, info); the experimental annotate,
// annotations_of(info, info) and annotation_of_type<T> are not provided.

#include <meta>

[[=1, =2.0f]] void fn();

static_assert(std::meta::annotations_of(^^fn).size() == 2);
static_assert(std::meta::annotations_of_with_type(^^fn, ^^int).size() == 1);
static_assert(std::meta::is_annotation(std::meta::annotations_of(^^fn)[0]));

consteval void test() {
  std::meta::annotate(^^fn, ^^int);                 // expected-error {{no member named 'annotate' in namespace 'std::meta'}}
  std::meta::annotation_of_type(^^fn);              // expected-error {{no member named 'annotation_of_type' in namespace 'std::meta'}}
  std::meta::annotations_of(^^fn, ^^int);           // expected-error {{too many arguments to function call}}
}
