// RUN: %clang_cc1 -std=c++2d -I %S/../Reflection/Inputs -Wno-undefined-inline -triple x86_64-linux-gnu -emit-llvm -o - %s | FileCheck %s --check-prefix=ITANIUM
// RUN: %clang_cc1 -std=c++2d -I %S/../Reflection/Inputs -Wno-undefined-inline -triple x86_64-windows-msvc -emit-llvm -o - %s | FileCheck %s --check-prefix=MSVC

// A dependent macro invocation in a function-template signature must be
// mangleable (it used to reach llvm_unreachable).
#include "token-sequence.h"
__macro inc(int x) { return ^^{ (\(x) + 1) }; }
template <class T> auto f(T t) -> decltype(inc!(t)) { return inc!(t); }
int use() { return f(1); }

// ITANIUM: @_Z1fIiEDTcl3incfp_EET_
// MSVC: @"??$f@H@@YAHH@Z"
