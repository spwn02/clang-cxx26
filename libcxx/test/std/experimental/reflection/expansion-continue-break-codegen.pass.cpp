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
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest -fexpansion-statements -O2 -g

// <experimental/reflection>
//
// [reflection]
//
// Regression test for upstream bloomberg/clang-p2996 issue #182: `template
// for` crashes at codegen if its body contains `continue` (a naked
// `continue`, or one reached via `if constexpr`), reproducing only at -O1
// and above (-O0 does not reproduce, per the upstream report) -- an LLVM
// IR-level crash in llvm::BranchInst::BranchInst / llvm::BasicBlock, also
// reported to sometimes surface later, in an "unused block" optimization
// pass.
//
// Extensive reproduction attempts against this fork -- matching the
// upstream repro's exact shape ("if constexpr() continue" and a bare
// "continue" at the top of the loop, both in template and non-template
// enclosing functions, at -O0 through -O3, with and without -g, with
// continue/break combined and with 1 to 10 iterations) were unable to
// reproduce any crash. Direct inspection of the lowering itself
// (CodeGenFunction::EmitCXXExpansionStmt, clang/lib/CodeGen/CGStmt.cpp)
// shows it already does the correct thing: one JumpDest is pre-allocated
// per expansion instance up front, and each instance's continue target is
// simply the next instance's JumpDest (or the loop's exit block for the
// last instance) -- there's no shared, instance-independent continuation
// destination for discarded branches to dangle a reference to.
//
// This item's own commit in docs/REFLECTION_CLOSEUP.md's item-6 row has
// the fuller account, including the most likely explanation: this exact
// function was dropped by a merge during this fork's LLVM 22 sync and
// later restored verbatim from the pre-merge fork (commit
// df8b70aeaf5e "restore expansion-statement CodeGen..."), which happened
// after the upstream #182 report (2025-09-08) -- plausibly picking up an
// LLVM-side or otherwise incidental fix along the way. This test exists to
// lock that state in: if this ever starts crashing again, this is the
// regression to bisect.

#include <cassert>
#include <meta>

// A bare 'continue' at the very start of the loop body -- the simplest
// shape the upstream issue's own top comment called out as crashing.
consteval int naked_continue_at_top() {
  int c = 0;
  template for (constexpr int i : {1, 2, 3}) {
    continue;
    c += i; // Never reached -- if this ran, the result below would be wrong.
  }
  return c;
}
static_assert(naked_continue_at_top() == 0);

// The exact shape from the issue title: 'if constexpr() continue'.
template <class T>
int if_constexpr_continue() {
  int c = 0;
  template for (constexpr int i : {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}) {
    if constexpr (i % 3 == 0) {
      continue;
    } else if constexpr (i % 5 == 0) {
      break;
    }
    c += i;
  }
  return c;
}

int main(int, char**) {
  // 1 + 2 (skip 3) + 4 (skip 5, break) = 7.
  assert((if_constexpr_continue<int>() == 7));
  assert((if_constexpr_continue<double>() == 7));
  return 0;
}
