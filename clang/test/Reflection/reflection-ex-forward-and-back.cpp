//===----------------------------------------------------------------------===//
//
// Copyright 2024 Bloomberg Finance L.P.
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// RUN: %clang_cc1 %s -std=c++23 -freflection
// NO-EXEC: compile-only splice-typename smoke test, nothing to execute or
// assert on -- main() is intentionally empty.

constexpr auto r = ^^int;
typename[:r:] x = 42;       // Same as: int x = 42;
typename[:^^char:] c = '*';  // Same as: char c = '*';

int main() {}
