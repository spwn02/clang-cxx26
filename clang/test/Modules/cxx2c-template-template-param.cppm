// RUN: rm -rf %t && mkdir -p %t
// RUN: split-file %s %t
// RUN: %clang_cc1 -std=c++2c -triple x86_64-linux-gnu -emit-module-interface %t/m.cppm -o %t/m.pcm
// RUN: %clang_cc1 -std=c++2c -triple x86_64-linux-gnu -fmodule-file=m=%t/m.pcm %t/u.cpp -emit-llvm -o - | FileCheck %t/u.cpp

//--- m.cppm
export module m;
export template <typename T> concept Small = sizeof(T) <= 4;
export template <typename T> constexpr auto Sz = sizeof(T);
export template <template <typename> concept C> struct Holder {
  template <typename T> requires C<T> static constexpr int f() { return 1; }
  template <typename T> requires (!C<T>) static constexpr int f() { return 2; }
};
export template <template <typename> auto V, typename T> constexpr auto get_v = V<T>;
export template <template <typename> auto V, typename T>
auto mg(T) -> decltype(V<T>) { return V<T>; }

//--- u.cpp
import m;
static_assert(Holder<Small>::f<int>() == 1);
static_assert(Holder<Small>::f<long long>() == 2);
static_assert(get_v<Sz, char> == 1);
long use() { return mg<Sz>(1); }
// CHECK: define {{.*}} @_ZW1m2mgIS_2SziEDT1VIT0_EES2_(
