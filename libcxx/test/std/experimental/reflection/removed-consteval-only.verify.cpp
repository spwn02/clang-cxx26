//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20 || c++23
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

// LWG4555 removes the public type-based consteval-only predicates.
#include <meta>
#include <type_traits>

using std::is_consteval_only; // expected-error {{no member named 'is_consteval_only' in namespace 'std'}}
using std::is_consteval_only_v; // expected-error {{no member named 'is_consteval_only_v' in namespace 'std'}}
using std::meta::is_consteval_only_type; // expected-error {{no member named 'is_consteval_only_type' in namespace 'std::meta'}}
