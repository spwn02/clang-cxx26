// RUN: %clang_cc1 -std=c++23 -ast-print %s | FileCheck %s

// -ast-print aborted on a bfloat16 literal ("Unexpected type for float
// literal!"), and so on every translation unit that includes <stdfloat> or
// <meta> (#499).

// CHECK: __bf16 x = 1.5BF16;
__bf16 x = 1.5bf16;
// CHECK: __bf16 y = 0.25BF16;
__bf16 y = 0.25BF16;
