// RUN: %clang_cc1 -std=c++2d -I %S/../../Reflection/Inputs -emit-pch -o %t.1.ast %S/Inputs/ts2.cpp
// RUN: %clang_cc1 -std=c++2d -ast-merge %t.1.ast -fsyntax-only -verify %s

// The literal tokens of an imported token sequence can be injected: the
// importer hands them to CreateTokenSequenceData, which gives each its own
// NUL-terminated storage in the destination context (#496).
// expected-no-diagnostics

consteval { inject_adjacent(); }
static_assert(arr[0] == 12345678 && arr[1] == 90123456 && arr[2] == 78);

consteval { inject_into_target(); }
static_assert(target::tgt[0] == 12345678 && target::tgt[1] == 90123456 && target::tgt[2] == 78);
