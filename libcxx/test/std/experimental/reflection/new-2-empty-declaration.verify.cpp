//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest
// ADDITIONAL_COMPILE_FLAGS: -fannotation-attributes

// M5 checklist row 3394-03 (P3394R4): an annotation cannot appear in an
// empty-declaration's attribute-specifier-seq.

[[=1]];
// expected-error@-1 {{'__annotation_placeholder' attribute cannot be applied to a declaration}}

[[=2]] int valid;
