// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// RUN: %clang_cc1 -std=c++26 -freflection -emit-pch -x c++-header %S/Inputs/cxx26-dependent-splice.h -o %t.pch
// RUN: %clang_cc1 -std=c++26 -freflection -include %S/Inputs/cxx26-dependent-splice.h -verify %s
// RUN: %clang_cc1 -std=c++26 -freflection -include-pch %t.pch -verify %s
// RUN: %clang_cc1 -std=c++26 -freflection -include-pch %t.pch -verify -emit-llvm %s -o %t.ll
// RUN: FileCheck %s --check-prefix=IR < %t.ll

// RUN: %clang_cc1 -std=c++26 -freflection -include-pch %t.pch -ast-dump-all -ast-dump-filter alias_target %s | FileCheck %s --check-prefix=ALIAS

// IR: define {{.*}}i32 @_Z4emitv()
// IR: define linkonce_odr {{.*}}5names
// IR: define linkonce_odr {{.*}}5specs
// IR: define linkonce_odr {{.*}}12alias_target
// IR: define linkonce_odr {{.*}}11alias_types
// IR: define linkonce_odr {{.*}}9qualified

// ALIAS: FunctionDecl {{.*}} imported {{.*}}constexpr alias_target
// ALIAS: NamespaceAliasDecl {{.*}} imported {{.*}}A
// ALIAS-NEXT: {{.*}}DependentNamespace {{.*}}
// ALIAS: NamespaceAliasDecl {{.*}} imported {{.*}}B
// ALIAS-NEXT: {{.*}}NamespaceAlias {{.*}} 'A'

// expected-no-diagnostics

static_assert(names<^^P>() == 0);
static_assert(specs<^^TT>() == 0);
static_assert(alias_target<^^N>() == 84);
static_assert(alias_types<^^N>() == 7);
static_assert(qualified<^^N>() == 84);
int emit() {
  return names<^^P>() + specs<^^TT>() + alias_target<^^N>() +
         alias_types<^^N>() + qualified<^^N>();
}
