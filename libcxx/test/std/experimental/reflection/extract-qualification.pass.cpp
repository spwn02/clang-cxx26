//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20
// ADDITIONAL_COMPILE_FLAGS: -freflection

// <meta>
//
// [meta.reflection.extract]: extract<T> accepts a qualification conversion from the type of the entity to T
// (pointer, pointer-to-member and reference results), function types that merely lose `noexcept`, and rejects
// a conversion that would drop a qualifier.

#include <meta>

using namespace std::meta;

int gi = 1;
const int gci = 2;
int garr[3] = {1, 2, 3};
void f() {}
void fn() noexcept {}
struct C {
  int m;
  void mf();
  void mfn() noexcept;
  static void sf();
};

template <class T>
consteval bool ok(info r) {
  try {
    (void)extract<T>(r);
    return true;
  } catch (std::meta::exception const&) {
    return false;
  }
}

// pointer values: qualification conversion is fine, dropping a qualifier is not
static_assert(ok<int*>(reflect_constant(&gi)));
static_assert(ok<const int*>(reflect_constant(&gi)));
static_assert(!ok<int*>(reflect_constant((const int*)&gi)));
static_assert(ok<const int*>(reflect_constant((const int*)&gi)));
static_assert(!ok<const int**>(reflect_constant((int**)nullptr))); // needs const at every intermediate level
static_assert(ok<const int* const*>(reflect_constant((int**)nullptr)));

// cv-unqualified value
static_assert(ok<int>(reflect_constant(5)));
static_assert(ok<const int>(reflect_constant(5)));

// references bind with a qualification conversion only
// [meta.reflection.extract] extract-ref: a variable that is not usable in
// constant expressions cannot be extracted by reference; an object reflection can.
static_assert(!ok<int&>(^^gi));
static_assert(ok<int&>(reflect_object(gi)));
static_assert(ok<const int&>(reflect_object(gi)));
static_assert(!ok<int&>(^^gci));
static_assert(ok<const int&>(^^gci));

// arrays decay
static_assert(ok<int*>(^^garr));
static_assert(ok<const int*>(^^garr));

// functions and member functions may lose noexcept but not gain it
static_assert(ok<void (*)()>(^^f));
static_assert(ok<void (*)() noexcept>(^^fn));
static_assert(ok<void (*)()>(^^fn));
static_assert(!ok<void (*)() noexcept>(^^f));
static_assert(ok<void (*)()>(^^C::sf));
static_assert(ok<void (C::*)()>(^^C::mf));
static_assert(ok<void (C::*)()>(^^C::mfn));
static_assert(ok<void (C::*)() noexcept>(^^C::mfn));
static_assert(!ok<void (C::*)() noexcept>(^^C::mf));

// data members
static_assert(ok<int C::*>(^^C::m));
static_assert(ok<const int C::*>(^^C::m));
static_assert(!ok<long C::*>(^^C::m));

// the extracted values are usable
constexpr int cxi = 1;
constexpr int cxarr[3] = {1, 2, 3};
static_assert(*extract<const int*>(reflect_constant(&cxi)) == 1);
static_assert(extract<const int&>(^^cxi) == 1);
static_assert(extract<const int*>(^^cxarr)[2] == 3);

// a caught exception, even by catch (...), is destroyed by the handler
consteval bool catch_all() {
  try {
    (void)extract<int>(^^::);
  } catch (...) {
    return true;
  }
  return false;
}
static_assert(catch_all());

int main(int, char**) { return 0; }
