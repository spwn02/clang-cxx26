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
// ADDITIONAL_COMPILE_FLAGS: -freflection
// ADDITIONAL_COMPILE_FLAGS: -fannotation-attributes

#include <meta>
#include <string>

// 3394-01: annotation operands must be constant expressions of structural type.
[[=std::string{"not structural"}]] void non_structural();
// expected-error@-1 {{C++26 annotation attribute requires a value of structural type}}

// 3394-04: annotation and ordinary attributes cannot share an attribute
// specifier.
[[=1, nodiscard]] void mixed_attributes();
// expected-error@-1 {{attribute specifier cannot contain both attributes and annotations}}

// 3394-06: repeated annotations preserve accumulation and order.
[[=1, =2]] void ordered();
static_assert(std::meta::extract<int>(std::meta::annotations_of(^^ordered)[0]) == 1);
static_assert(std::meta::extract<int>(std::meta::annotations_of(^^ordered)[1]) == 2);

int main() {}
