//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <simd>

// [simd.expos] makes simd-size-type, native-abi, deduce-abi-t, simd-size-v, and
// mask-element-size exposition-only. A conforming program is free to declare its own names with
// those exact spellings in namespace std::simd; the header must not have already claimed them.
//
// static_assert can't test that a name is *absent* -- there's nothing to write an assertion about.
// This instead declares a fresh entity under each name directly inside namespace std::simd: if the
// header leaked any of these publicly, redeclaring the name here collides with it and this file
// fails to compile with a redefinition/redeclaration-mismatch diagnostic. Since none of them should
// be public, no diagnostic is expected.
//
// This mirrors the audit's own §1 finding: all five names were public in the pre-port prototype
// (__simd/simd.h, now retired) and were renamed to the __-prefixed exposition-only spelling used in
// the current headers precisely so this file can pass.
// https://claude.ai/code/artifact/ec199a8d-5630-4a21-a780-1b5aef8322ad#s1

#include <cstddef>
#include <simd>

// expected-no-diagnostics

namespace std::simd {

using simd_size_type = int;

template <class T>
using native_abi = void;

template <class T, int N>
using deduce_abi_t = void;

template <class T, class Abi>
inline constexpr int simd_size_v = 0;

template <class T>
inline constexpr std::size_t mask_element_size = 0;

} // namespace std::simd
