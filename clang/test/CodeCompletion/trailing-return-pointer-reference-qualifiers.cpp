// After the pointer or reference operator that ends a trailing return type the
// rest of the function declarator follows (noexcept, requires, pre, post), as
// after `int` in `auto f() -> int <cursor>`
// (contract-specifiers-trailing-return.cpp). The qualifier list of the
// operator was completed with const and volatile only, so `pre` typed after
// `-> const T &` had nothing to complete to.

template <class T> struct S {
  auto ref() const -> const T & 
  ;
  auto ptr() -> T * 
  ;
  auto rref() -> T && 
  ;
};
auto plain() -> int * 

// RUN: %clang_cc1 -fsyntax-only -code-completion-patterns -code-completion-at=%s:9:33 -std=c++26 %s -o - | FileCheck %s
// RUN: %clang_cc1 -fsyntax-only -code-completion-patterns -code-completion-at=%s:11:21 -std=c++26 %s -o - | FileCheck %s
// RUN: %clang_cc1 -fsyntax-only -code-completion-patterns -code-completion-at=%s:13:23 -std=c++26 %s -o - | FileCheck %s
// RUN: %clang_cc1 -fsyntax-only -code-completion-patterns -code-completion-at=%s:16:23 -std=c++26 %s -o - | FileCheck %s
// CHECK-DAG: COMPLETION: const
// CHECK-DAG: COMPLETION: noexcept
// CHECK-DAG: COMPLETION: Pattern : post(<#expression#>)
// CHECK-DAG: COMPLETION: Pattern : pre(<#expression#>)
// CHECK-DAG: COMPLETION: requires
// CHECK-DAG: COMPLETION: volatile

// Before C++26, and before C++20.
// RUN: %clang_cc1 -fsyntax-only -code-completion-patterns -code-completion-at=%s:9:33 -std=c++26 %s -o - | FileCheck --check-prefix=NOCONTRACTS %s
// NOCONTRACTS-NOT: pre(<#expression#>)
// NOCONTRACTS-DAG: COMPLETION: noexcept
// NOCONTRACTS-DAG: COMPLETION: requires
// RUN: %clang_cc1 -fsyntax-only -code-completion-patterns -code-completion-at=%s:9:33 -std=c++17 %s -o - | FileCheck --check-prefix=CXX17 %s
// CXX17-NOT: requires
// CXX17-DAG: COMPLETION: noexcept
