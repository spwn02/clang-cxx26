//===----------------------------------------------------------------------===//
//
// Copyright 2026 Jump Trading, LLC
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20 || c++23 || c++26

// punctuator_of takes a single punctuator token.

#include <meta>

using std::meta::punctuator;

constexpr punctuator a = punctuator_of((^^{ x })[0]); // #a
// expected-error@#a {{constexpr variable 'a' must be initialized by a constant expression}}
// expected-error@#a {{call to consteval function 'std::meta::punctuator_of' is not a constant expression}}
constexpr punctuator b = punctuator_of(std::meta::token{}); // #b
// expected-error@#b {{constexpr variable 'b' must be initialized by a constant expression}}
// expected-error@#b {{call to consteval function 'std::meta::punctuator_of' is not a constant expression}}
constexpr punctuator c = punctuator_of((^^{ 42 })[0]); // #c
// expected-error@#c {{constexpr variable 'c' must be initialized by a constant expression}}
// expected-error@#c {{call to consteval function 'std::meta::punctuator_of' is not a constant expression}}
// expected-note@*:* 3+ {{}}
