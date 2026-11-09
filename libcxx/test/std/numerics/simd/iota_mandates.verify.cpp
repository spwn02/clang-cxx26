//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23, c++26

// <simd>

// [simd.creation] (P3319R6): iota<T> Mandates that T is a vectorizable arithmetic type or an enabled basic_vec of an
// arithmetic type that can represent size() - 1.

#include <simd>
#include <string>

auto not_vectorizable = std::simd::iota<bool>;
// expected-error@*:* {{simd::iota<T>: an arithmetic T must be a vectorizable type}}

auto not_simd = std::simd::iota<std::string>;
// expected-error@*:* {{simd::iota<T>: T must be a vectorizable arithmetic type or an enabled specialization of basic_vec}}
// expected-error@*:* {{constexpr variable 'iota<std::string>' must be initialized by a constant expression}}
