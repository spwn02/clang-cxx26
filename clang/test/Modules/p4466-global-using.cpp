// RUN: mkdir -p %t
// RUN: %clang_cc1 -std=c++26 -emit-module-interface -o %t/lib26.pcm %S/Inputs/p4466-global-using.cppm
// RUN: %clang_cc1 -std=c++26 -fmodule-file=p4466_global_using=%t/lib26.pcm %s -verify
import p4466_global_using;

static_assert(^^::size_t == ^^size_t);
static_assert(^^::strlen == ^^strlen);

// expected-no-diagnostics
