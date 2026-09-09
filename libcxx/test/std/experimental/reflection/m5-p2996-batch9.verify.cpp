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

namespace p2996_batch9 {

constexpr auto bad_offset = std::meta::offset_of(^^int);
// expected-error@-1 {{must be initialized by a constant expression}}
constexpr auto incomplete_size = std::meta::size_of(^^void);
// expected-error@-1 {{must be initialized by a constant expression}}
constexpr auto invalid_size = std::meta::size_of(^^::);
// expected-error@-1 {{must be initialized by a constant expression}}
constexpr auto incomplete_alignment = std::meta::alignment_of(^^void);
// expected-error@-1 {{must be initialized by a constant expression}}
constexpr auto invalid_alignment = std::meta::alignment_of(^^::);
// expected-error@-1 {{must be initialized by a constant expression}}
constexpr auto incomplete_bit_size = std::meta::bit_size_of(^^void);
// expected-error@-1 {{must be initialized by a constant expression}}
constexpr auto invalid_bit_size = std::meta::bit_size_of(^^::);
// expected-error@-1 {{must be initialized by a constant expression}}

} // namespace p2996_batch9
