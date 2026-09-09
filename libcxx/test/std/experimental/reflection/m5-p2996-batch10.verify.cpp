//===----------------------------------------------------------------------===//
//
// Copyright 2026
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

#include <meta>

namespace p2996_batch10 {

constexpr auto bad_extract_type = std::meta::extract<int>(^^int);
// expected-error@-1 {{must be initialized by a constant expression}}
constexpr auto bad_extract_value = std::meta::extract<float>(
    std::meta::reflect_constant(42));
// expected-error@-2 {{must be initialized by a constant expression}}
constexpr auto bad_extract_pointer = std::meta::extract<int *>(
    std::meta::reflect_constant(42));
// expected-error@-2 {{must be initialized by a constant expression}}
constexpr auto bad_can_substitute = std::meta::can_substitute(^^int, {});
// expected-error@-1 {{must be initialized by a constant expression}}
constexpr auto bad_substitute = std::meta::substitute(^^int, {});
// expected-error@-1 {{must be initialized by a constant expression}}

} // namespace p2996_batch10
