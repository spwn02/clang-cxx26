// RUN: %clang_cc1 -std=c++2d -I %S/../Reflection/Inputs -emit-pch -x c++-header %S/Inputs/deferred-mem-init-macro.h -o %t.pch
// RUN: %clang_cc1 -std=c++2d -include-pch %t.pch -emit-pch -x c++-header %S/Inputs/deferred-mem-init-macro-empty.h -o %t.chained.pch
// RUN: %clang_cc1 -std=c++2d -I %S/../Reflection/Inputs -include %S/Inputs/deferred-mem-init-macro.h -fsyntax-only %s
// RUN: %clang_cc1 -std=c++2d -include-pch %t.pch -fsyntax-only %s
// RUN: %clang_cc1 -std=c++2d -include-pch %t.chained.pch -fsyntax-only %s
// The constexpr check below exercises the initializer both directly and after
// the constructor's deferred initializer has been imported from the PCH.

static_assert(DeferredCtorInit<int>(17).member == 17);
int main() { DeferredCtorInit<int> value(17); return value.member != 17; }
