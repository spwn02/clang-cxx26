// RUN: %clang_cc1 -triple x86_64-pc-windows-msvc -fms-compatibility -fms-extensions -std=c++2c -emit-llvm -o %t.ll %s
// RUN: %clang_cc1 -triple x86_64-pc-windows-msvc -fms-compatibility -fms-extensions -std=c++2c -x c++-header -emit-pch -o %t.pch %S/Inputs/ms-mangle-reflection-unnamed.h
// RUN: %clang_cc1 -triple x86_64-pc-windows-msvc -fms-compatibility -fms-extensions -std=c++2c -include-pch %t.pch -emit-llvm -o %t.pch.ll %s
// RUN: FileCheck %s < %t.ll
// RUN: FileCheck %s < %t.pch.ll
// RUN: FileCheck %s --check-prefix=SYM < %t.ll
// RUN: FileCheck %s --check-prefix=SYM < %t.pch.ll

// Members of different unnamed records, template parameters and template
// template parameters are distinct reflections and keep distinct Microsoft
// symbols (the Microsoft ABI's own unnamed-tag numbering does not distinguish
// the unnamed union from the unnamed struct); a collision is a redefinition
// error, so compiling and counting the definitions is the check.
#include "Inputs/ms-mangle-reflection-unnamed.h"
using info = decltype(^^int);
template <info R> int code() { return 0; }
template <class T> int tp() { return code<^^T>(); }
template <template <class> class TT> int ttp() { return code<^^TT>(); }
int an1 = code<^^Anon::a>();
int an2 = code<^^Anon::b>();
int an3 = code<^^Anon::c>();
int an4 = code<^^Anon::d>();
int an5 = code<^^Anon::e>();
int use = tp<int>() + tp<long>() + tp<Box<int>>() + ttp<Box>();
// CHECK-COUNT-9: define {{.*}}??$code@
// SYM-DAG: define {{.*}}@{{"?}}??$code@$$$r19_4d64665f5a54534e34416e6f6e55745f453045@@YAHXZ
// SYM-DAG: define {{.*}}@{{"?}}??$code@$$$r19_4d64665f5a54534e34416e6f6e55745f453145@@YAHXZ
// SYM-DAG: define {{.*}}@{{"?}}??$code@$$$r20_4d64665f5a54534e34416e6f6e5574305f453045@@YAHXZ
// SYM-DAG: define {{.*}}@{{"?}}??$code@$$$r20_4d64665f5a54534e34416e6f6e5574305f453145@@YAHXZ
// SYM-DAG: define {{.*}}@{{"?}}??$code@$$$r20_4d64665f5a54534e34416e6f6e5574315f453045@@YAHXZ
