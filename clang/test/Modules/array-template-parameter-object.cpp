// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// RUN: split-file %s %t
// RUN: %clang_cc1 -std=c++26 -freflection -I %S/../CodeGenCXX/Inputs -emit-module-interface %t/array.cppm -o %t/array.pcm
// RUN: %clang_cc1 -std=c++26 -freflection -I %S/../CodeGenCXX/Inputs -fmodule-file=ArrayTPO=%t/array.pcm -verify %t/use.cpp

//--- array.cppm
module;
#include "array-template-parameter-object.h"
export module ArrayTPO;
export consteval info module_object() { return r; }
export consteval const int* module_address() { return pointer; }
export consteval info from_values(const int* data) {
  return array_object(dims, 1, data, 3);
}
export consteval const int* address_of(info value) {
  return extract<const int*>(value);
}
export consteval auto reference_of(info value) -> const int(&)[3] {
  return extract<const int(&)[3]>(value);
}

//--- use.cpp
// expected-no-diagnostics
import ArrayTPO;
constexpr int data[] = {1, 2, 3};
constexpr auto rebuilt = from_values(data);
static_assert(module_object() == rebuilt);
static_assert(module_address() == address_of(rebuilt));
static_assert(reference_of(module_object())[2] == 3);
template <const int(&)[3]> struct W {};
static_assert(__is_same(W<reference_of(module_object())>,
                       W<reference_of(rebuilt)>));
