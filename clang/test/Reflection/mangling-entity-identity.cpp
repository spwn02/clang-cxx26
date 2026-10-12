// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++26 -freflection -fparameter-reflection -emit-llvm -o %t.plain.ll %s
// RUN: FileCheck %s < %t.plain.ll
// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++26 -freflection -fparameter-reflection -x c++-header -emit-pch -o %t.pch %S/Inputs/reflection-entity-identity.h
// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++26 -freflection -fparameter-reflection -include-pch %t.pch -emit-llvm -o %t.pch.ll %s
// RUN: FileCheck %s < %t.pch.ll

#include "Inputs/reflection-entity-identity.h"
int enums() {
  return code<^^first_enum>() + code<^^second_enum>() +
         code<^^Scoped::first>() + code<^^Scoped::second>() +
         code<^^first_anonymous>() + code<^^second_anonymous>();
}
int bases() {
  return code<d_base>() + code<e_base>() + code<multiple_first>() +
         code<multiple_second>();
}
int parameters() {
  return code<first_parameter>() + code<second_parameter>() +
         code<other_parameter>() + code<first_constructor_parameter>() +
         code<second_constructor_parameter>();
}
int main() {
  return enums() + bases() + parameters() == 120 ? 0 : 1;
}

// Distinct reflected entities/relationships keep distinct specializations,
// even when their enum values, base types, or parameter ordinals coincide.
// CHECK-DAG: define {{.*}}@_Z4codeIMd10first_enumEEiv
// CHECK-DAG: define {{.*}}@_Z4codeIMd11second_enumEEiv
// CHECK-DAG: define {{.*}}@_Z4codeIMdN6Scoped5firstEEEiv
// CHECK-DAG: define {{.*}}@_Z4codeIMdN6Scoped6secondEEEiv
// CHECK-DAG: define {{.*}}@_Z4codeIMd15first_anonymousEEiv
// CHECK-DAG: define {{.*}}@_Z4codeIMd16second_anonymousEEiv
// CHECK-DAG: define {{.*}}@_Z4codeIMb_ZTS1D0_EEiv
// CHECK-DAG: define {{.*}}@_Z4codeIMb_ZTS2E20_EEiv
// CHECK-DAG: define {{.*}}@_Z4codeIMb_ZTS8Multiple0_EEiv
// CHECK-DAG: define {{.*}}@_Z4codeIMb_ZTS8Multiple1_EEiv
// CHECK-DAG: define {{.*}}@_Z4codeIMpMd_Z14first_functionidE0_EEiv
// CHECK-DAG: define {{.*}}@_Z4codeIMpMd_Z14first_functionidE1_EEiv
// CHECK-DAG: define {{.*}}@_Z4codeIMpMd_Z15second_functionidE0_EEiv
// CHECK-DAG: define {{.*}}@_Z4codeIMpMd_ZN1CC1EiE0_EEiv
// CHECK-DAG: define {{.*}}@_Z4codeIMpMd_ZN1CC1EdE0_EEiv
