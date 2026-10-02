// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20 || c++23
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

// annotations_of order (TU order across redeclarations), parameter-variable
// scoping, function-template specialization chains, pack-expanded annotation
// lists (#179 gap G, #180).

#include <meta>
using namespace std::meta;
[[=10]] void h(); [[=20]] void h();
static_assert(extract<int>(annotations_of(^^h)[0])==10 && extract<int>(annotations_of(^^h)[1])==20);
[[=2,=3,=2]] void g(); void g [[=4,=5]] ();
static_assert(annotations_of(^^g).size()==5 && extract<int>(annotations_of(^^g)[2])==2 && extract<int>(annotations_of(^^g)[3])==4);
void p([[=1]] int x); void p([[=2]] [[maybe_unused]] int y){}
static_assert(extract<int>(annotations_of(parameters_of(^^p)[0])[0])==1 && extract<int>(annotations_of(parameters_of(^^p)[0])[1])==2);
void f([[=1]] int x);
void f([[=2]] [[maybe_unused]] int y) {
  static_assert(annotations_of(parameters_of(^^f)[0]).size() == 2);
  static_assert(annotations_of(variable_of(parameters_of(^^f)[0])).size() == 1);
}
template<class T> [[=1]] void ft(T); template<class T> void ft [[=2]] (T){} template<> [[=3]] void ft<int>(int){}
static_assert(annotations_of(^^ft<long>).size()==2 && extract<int>(annotations_of(^^ft<long>)[0])==1);
static_assert(annotations_of(^^ft<int>).size()==3 && extract<int>(annotations_of(^^ft<int>)[2])==3);
template<int... Is> struct [[=Is...]] P {}; static_assert(annotations_of(^^P<1,2,3>).size()==3);

template<class... Ts> struct [[=sizeof(Ts)...]] P3 {};
static_assert(annotations_of(^^P3<char, int>).size() == 2 &&
              extract<unsigned long>(annotations_of(^^P3<char, int>)[0]) == 1 &&
              extract<unsigned long>(annotations_of(^^P3<char, int>)[1]) == 4);
static_assert(annotations_of(^^P3<>).size() == 0);

int main(int, char**) { return 0; }
