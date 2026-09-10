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
// ADDITIONAL_COMPILE_FLAGS: -Xclang -verify-ignore-unexpected=note

#include <meta>

constexpr auto bad_function = std::meta::current_function();
// expected-error@-1 {{must be initialized by a constant expression}}
// expected-note@-2 {{in call to 'current_function()'}}

constexpr auto bad_class = std::meta::current_class();
// expected-error@-1 {{must be initialized by a constant expression}}
// expected-note@-2 {{in call to 'current_class()'}}

constexpr auto global_namespace = std::meta::current_namespace();
static_assert(global_namespace == ^^::);
