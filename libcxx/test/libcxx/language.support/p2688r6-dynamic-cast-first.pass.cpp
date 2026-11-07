//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20 || c++23 || c++26
// UNSUPPORTED: no-rtti

// ADDITIONAL_COMPILE_FLAGS: -stdlib=libc++

// RUN: %{build}
// RUN: %{exec} %t.exe

// A pattern matching selection over type patterns selects the first case whose
// dynamic_cast succeeds. With libc++ the compiler tries them in one call to
// libc++abi's __dynamic_cast_first (a link error here would be a disagreement
// between the compiler and the runtime); the result must be what the
// sequential casts select.

#include <cassert>

struct Shape { virtual ~Shape() {} };
struct Circle : Shape {};
struct Triangle : Shape {};
struct Rectangle : Shape {};
struct Square : Rectangle {};
struct Disk : Circle {};
struct Both : Circle, Triangle {};

static int by_match(Shape& s) {
  return match (s) {
    case const Circle&    => 1;
    case const Triangle&  => 2;
    case const Rectangle& => 3;
    case _                => 0;
  };
}
static int by_casts(Shape& s) {
  if (dynamic_cast<Circle*>(&s))
    return 1;
  if (dynamic_cast<Triangle*>(&s))
    return 2;
  if (dynamic_cast<Rectangle*>(&s))
    return 3;
  return 0;
}

// A more derived dynamic type listed after its base: the target order decides.
static int by_match_order(Shape& s) {
  return match (s) {
    case const Circle& => 1;
    case const Disk&   => 2;
    case _             => 0;
  };
}
static int by_casts_order(Shape& s) {
  if (dynamic_cast<Circle*>(&s))
    return 1;
  if (dynamic_cast<Disk*>(&s))
    return 2;
  return 0;
}

// Virtual and repeated bases.
struct Root { virtual ~Root() {} };
struct A : virtual Root {};
struct L : A {};
struct R : A {};
struct Object : L, R {};

static int by_match_virtual(Root& r) {
  return match (r) {
    case const A&      => 1;
    case const L&      => 2;
    case const R&      => 3;
    case const Object& => 4;
    case _             => 0;
  };
}
static int by_casts_virtual(Root& r) {
  if (dynamic_cast<A*>(&r))
    return 1;
  if (dynamic_cast<L*>(&r))
    return 2;
  if (dynamic_cast<R*>(&r))
    return 3;
  if (dynamic_cast<Object*>(&r))
    return 4;
  return 0;
}

int main(int, char**) {
  Shape shape;
  Circle circle;
  Triangle triangle;
  Rectangle rectangle;
  Square square;
  Disk disk;
  Both both;
  Shape* shapes[] = {&shape,  &circle, &triangle,
                     &rectangle, &square, &disk,
                     static_cast<Circle*>(&both), static_cast<Triangle*>(&both)};
  for (Shape* s : shapes) {
    assert(by_match(*s) == by_casts(*s));
    assert(by_match_order(*s) == by_casts_order(*s));
  }
  assert(by_match(shape) == 0);
  assert(by_match(circle) == 1);
  assert(by_match(square) == 3);
  assert(by_match_order(disk) == 1);

  Object object;
  L l;
  R r;
  A a;
  Root root;
  Root* roots[] = {&root, &a, &l, &r, &object};
  for (Root* p : roots)
    assert(by_match_virtual(*p) == by_casts_virtual(*p));
  return 0;
}
