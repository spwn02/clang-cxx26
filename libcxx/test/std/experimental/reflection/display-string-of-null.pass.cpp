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
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

// <experimental/reflection>
//
// [reflection]
//
// M5 checklist row 2996-09 (P2996R13 [meta.reflection.names]):
// display_string_of(r) and u8display_string_of(r) must return an
// unspecified but non-empty string_view for every reflection, including
// the null reflection. (An earlier session mistakenly flagged this as a
// gap by reading source rather than testing -- see the refuted NEW-1
// entry in docs/REFLECTION_GAPS.md; this fork's existing
// tprint_impl::render<R>() null specialization already handles it
// correctly. This test exists to give that guarantee real coverage.)

#include <meta>

static_assert(std::meta::display_string_of(std::meta::info{}).size() > 0);
static_assert(std::meta::u8display_string_of(std::meta::info{}).size() > 0);
static_assert(std::meta::display_string_of(^^int).size() > 0);

int main() {
  return 0;
}
