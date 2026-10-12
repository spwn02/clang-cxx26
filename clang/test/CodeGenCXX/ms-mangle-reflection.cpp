// RUN: %clang_cc1 -triple x86_64-pc-windows-msvc -fms-compatibility -fms-extensions -std=c++2c -freflection -fparameter-reflection -fannotation-attributes -emit-llvm -o %t.ms.ll %s
// RUN: FileCheck %s --check-prefix=MS < %t.ms.ll
// RUN: %clang_cc1 -triple x86_64-pc-windows-msvc -fms-compatibility -fms-extensions -std=c++2c -freflection -fparameter-reflection -fannotation-attributes -x c++-header -emit-pch -o %t.pch %S/Inputs/ms-mangle-reflection.h
// RUN: %clang_cc1 -triple x86_64-pc-windows-msvc -fms-compatibility -fms-extensions -std=c++2c -freflection -fparameter-reflection -fannotation-attributes -include-pch %t.pch -emit-llvm -o %t.pch.ll %s
// RUN: FileCheck %s --check-prefix=MS < %t.pch.ll
// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++26 -freflection -fparameter-reflection -fannotation-attributes -emit-llvm -o %t.itanium.ll %s
// RUN: FileCheck %s --check-prefix=ITANIUM < %t.itanium.ll

#include "Inputs/ms-mangle-reflection.h"

holder<^^int> reflected_int;
holder<^^long> reflected_long;

int first_base = code<reflected_base(^^first)>();
int second_base = code<reflected_base(^^second)>();

enum values { first_value = 1, second_value = 1 };
int first_enum = code<^^first_value>();
int second_enum = code<^^second_value>();

[[=1]] int first_annotated;
[[=1]] int second_annotated;
int first_annotation = code<reflected_annotation(^^first_annotated)>();
int second_annotation = code<reflected_annotation(^^second_annotated)>();

int parameters(int first_parameter, int second_parameter);
int first_parameter = code<reflected_parameter(^^parameters, 0)>();
int second_parameter = code<reflected_parameter(^^parameters, 1)>();

// The MS framing uses an ASCII-only payload and produces distinct symbols.
// MS: @{{"?}}?reflected_int@@3U?$holder@$M$$M$$r8_4d745f5a54536945@@A
// MS: @{{"?}}?reflected_long@@3U?$holder@$M$$M$$r8_4d745f5a54536c45@@A
// MS: ??$code@$M$$M$$r15_4d625f5a5453356669727374305f45@@YAHXZ
// MS: ??$code@$M$$M$$r16_4d625f5a5453367365636f6e64305f45@@YAHXZ
// MS: ??$code@$M$$M$$r16_4d64313166697273745f76616c756545@@YAHXZ
// MS: ??$code@$M$$M$$r17_4d6431327365636f6e645f76616c756545@@YAHXZ
// MS: ??$code@$M$$M$$r33_4d614d645f5a313566697273745f616e6e6f746174656445305f305f4c69314545@@YAHXZ
// MS: ??$code@$M$$M$$r34_4d614d645f5a31367365636f6e645f616e6e6f746174656445305f305f4c69314545@@YAHXZ
// MS: ??$code@$M$$M$$r24_4d704d645f5a3130706172616d6574657273696945305f45@@YAHXZ
// MS: ??$code@$M$$M$$r24_4d704d645f5a3130706172616d6574657273696945315f45@@YAHXZ
// ITANIUM: @reflected_int =
// ITANIUM: @reflected_long =
