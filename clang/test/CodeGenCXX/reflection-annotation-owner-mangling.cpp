// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++26 -freflection -fparameter-reflection -fannotation-attributes -emit-llvm -o - %s | FileCheck %s

// The annotation reflection of an enumeration, a typedef or an alias declaration is mangled (the mangler had no
// encoding for the declaration of an owner that is none of a class, a namespace, a function or a variable, and
// the compiler aborted when such a reflection was a template argument).

#include "Inputs/ms-mangle-reflection.h"

enum [[=1]] plain_enum { plain_value };
enum class [[=2]] scoped_enum { scoped_value };
typedef int [[=3]] typedef_name;
using alias_name [[=4]] = int;
struct [[=5]] record_owner {};
enum [[=6, =7]] two_annotations { two_value };

int a1 = code<reflected_annotation(^^plain_enum)>();
int a2 = code<reflected_annotation(^^scoped_enum)>();
int a3 = code<reflected_annotation(^^typedef_name)>();
int a4 = code<reflected_annotation(^^alias_name)>();
int a5 = code<reflected_annotation(^^record_owner)>();
int a6 = code<reflected_annotation(^^two_annotations, 0)>();
int a7 = code<reflected_annotation(^^two_annotations, 1)>();

// One symbol per annotation (the enumeration's annotations are told apart by their position).
// CHECK-DAG: define {{.*}}@_Z4codeITnDaMad_ZTS10plain_enum0_0_Li1EEEiv
// CHECK-DAG: define {{.*}}@_Z4codeITnDaMad_ZTS11scoped_enum0_0_Li2EEEiv
// CHECK-DAG: define {{.*}}@_Z4codeITnDaMay12typedef_name0_0_Li3EEEiv
// CHECK-DAG: define {{.*}}@_Z4codeITnDaMay10alias_name0_0_Li4EEEiv
// CHECK-DAG: define {{.*}}@_Z4codeITnDaMad_ZTS12record_owner0_0_Li5EEEiv
// CHECK-DAG: define {{.*}}@_Z4codeITnDaMad_ZTS15two_annotations0_0_Li6EEEiv
// CHECK-DAG: define {{.*}}@_Z4codeITnDaMad_ZTS15two_annotations0_1_Li7EEEiv
