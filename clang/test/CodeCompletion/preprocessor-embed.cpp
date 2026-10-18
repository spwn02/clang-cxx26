// #embed is a directive of C++26 (and of C23): it is offered after '#'.
#e
// RUN: %clang_cc1 -std=c++26 -fsyntax-only -code-completion-at=%s:2:3 %s | FileCheck -check-prefix=CHECK-CXX26 %s
// CHECK-CXX26: COMPLETION: Pattern : embed "<#file#>"
// CHECK-CXX26: COMPLETION: Pattern : embed <<#file#>>
// RUN: %clang_cc1 -std=c++23 -fsyntax-only -code-completion-at=%s:2:3 %s | FileCheck -check-prefix=CHECK-CXX23 %s
// CHECK-CXX23-NOT: embed
// CHECK-CXX23: COMPLETION: Pattern : error <#message#>
