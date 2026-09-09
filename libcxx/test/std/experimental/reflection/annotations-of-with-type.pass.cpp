//===----------------------------------------------------------------------===//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// ADDITIONAL_COMPILE_FLAGS: -freflection -fannotation-attributes

#include <meta>

[[=1]] void annotated();

static_assert(std::meta::annotations_of_with_type(^^annotated, ^^int).size() == 1);
static_assert(std::meta::annotations_of_with_type(^^annotated, ^^float).size() == 0);

int main() {}
