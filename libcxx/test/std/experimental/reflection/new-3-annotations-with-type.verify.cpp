//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

// M5 checklist row 3394-05 (P3394R4): annotations_of_with_type(item, type)
// requires type to be a complete reflected type.

#include <meta>

constexpr auto item = ^^int;
constexpr auto incomplete_type = ^^void;
constexpr auto bad = std::meta::annotations_of_with_type(item,
                                                           incomplete_type);
// expected-error@-2 {{must be initialized by a constant expression}}

constexpr auto valid_type = ^^int;
constexpr auto good = std::meta::annotations_of_with_type(item, valid_type);
static_assert(good.empty());
