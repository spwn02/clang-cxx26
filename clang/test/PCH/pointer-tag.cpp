// RUN: %clang_cc1 -std=c++2c -emit-pch -o %t.pch %s
// RUN: %clang_cc1 -std=c++2c -include-pch %t.pch -fsyntax-only -verify %s -DUSE
// RUN: %clang_cc1 -std=c++2c -include-pch %t.pch -emit-llvm -o - %s -DUSE | FileCheck %s

// P3125R6: a tagged pointer in a constant survives a PCH round trip: the tag is part of the stored value.

#ifndef USE
alignas(8) constexpr int G = 1;
constexpr const void *Tagged = __builtin_pointer_tag_insert(&G, 5, 3);
struct Holder {
  const void *p;
};
constexpr Holder H{__builtin_pointer_tag_insert(&G, 6, 3)};
#else
// expected-no-diagnostics
static_assert(__builtin_pointer_tag_get(Tagged, 3) == 5);
static_assert(__builtin_pointer_tag_strip(Tagged, 3) == &G);
static_assert(__builtin_pointer_tag_get(H.p, 3) == 6);
static_assert(Tagged != H.p);
const void *Use = Tagged;
// CHECK: @Use = {{.*}}global ptr getelementptr (i8, ptr @_ZL1G, i64 5)
#endif
