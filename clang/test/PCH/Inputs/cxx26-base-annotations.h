// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef CXX26_BASE_ANNOTATIONS_H
#define CXX26_BASE_ANNOTATIONS_H

using info = decltype(^^int);
struct sentinel {};
consteval info base(info r, unsigned i = 0) {
  return __metafunction(2, r, ^^sentinel, i, ^^base);
}
consteval info annotation(info r, unsigned i) {
  return __metafunction(114, r, ^^sentinel, i, ^^annotation);
}
consteval int value(info r, unsigned i) {
  return __metafunction(25, ^^int, annotation(r, i), ^^value);
}

consteval info constant(info r) {
  return __metafunction(22, r, ^^constant);
}
consteval bool is_annotation(info r) {
  return __metafunction(115, r);
}

struct B0 {};
struct B1 {};
struct D1 : [[=13, =21, =13]] virtual public B0, B1 {};
template <int N> struct Dep : [[=N]] protected B0 {};
template <class... Bs> struct Pack : [[=1]] Bs... {};
template <int... Ns> struct AnnotationPack : [[=Ns...]] B0 {};

// Serialize both an existing specialization and an uninstantiated pattern.
static_assert(value(base(^^Dep<2>), 0) == 2);
static_assert(value(base(^^Pack<B0>), 0) == 1);
static_assert(value(base(^^AnnotationPack<4, 5>), 1) == 5);

#endif
