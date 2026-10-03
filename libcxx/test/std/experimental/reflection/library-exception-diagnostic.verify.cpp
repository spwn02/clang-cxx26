// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20
// ADDITIONAL_COMPILE_FLAGS: -freflection

#include <meta>

namespace ns {}

static_assert((std::meta::size_of(^^void), true));
// expected-error@-1 {{static assertion expression is not an integral constant expression}}
// expected-note@*:*  {{reflection failure: size_of requires a complete type, object, value, non-reference variable, non-bit-field member or direct base}}

static_assert((std::meta::bases_of(^^ns, std::meta::access_context::unchecked()), true));
// expected-error@-1 {{static assertion expression is not an integral constant expression}}
// expected-note@*:*  {{reflection failure: bases_of requires a class type, not a namespace}}

static_assert((std::meta::substitute(^^int, {}), true));
// expected-error@-1 {{static assertion expression is not an integral constant expression}}
// expected-note@*:*  {{reflection failure: expected a reflection of a template, but got a type}}
