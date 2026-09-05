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
// RUN: %clangxx %s -std=c++23 -freflection -o %t
// RUN: %t

struct S { unsigned i:2, j:6; };

consteval auto member_number(int n) {
  if (n == 0) return ^^S::i;
  else if (n == 1) return ^^S::j;
}

int main() {
  S s{0, 0};
  s.[:member_number(1):] = 42;  // Same as: s.j = 42;
  // This used to only be compiled, never executed or checked against the
  // value the comment above claims (docs/CONTRACTS_HARDENING.md M4's
  // coverage gate) -- confirm the splice-assignment actually reached `j`,
  // and that `i` (a different bitfield in the same storage unit) is
  // unaffected.
  return (s.j == 42 && s.i == 0) ? 0 : 1;
}
