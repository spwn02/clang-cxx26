// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20
// ADDITIONAL_COMPILE_FLAGS: -freflection
// ADDITIONAL_COMPILE_FLAGS: -fannotation-attributes

// P3394R4: annotation-list pack expansions preserve each expanded value.

#include <meta>
using namespace std::meta;
template<int... Is> struct [[=0, =Is..., =9]] Values {};
static_assert(annotations_of(^^Values<>).size() == 2);
static_assert(annotations_of(^^Values<1, 2>).size() == 4);
static_assert(extract<int>(annotations_of(^^Values<1, 2>)[0]) == 0);
static_assert(extract<int>(annotations_of(^^Values<1, 2>)[1]) == 1);
static_assert(extract<int>(annotations_of(^^Values<1, 2>)[2]) == 2);
static_assert(extract<int>(annotations_of(^^Values<1, 2>)[3]) == 9);
template<class... Ts> struct [[=sizeof(Ts)...]] Sizes {};
static_assert(annotations_of(^^Sizes<>).empty());
static_assert(extract<decltype(sizeof(int))>(
                  annotations_of(^^Sizes<char, int>)[0]) == 1);
static_assert(extract<decltype(sizeof(int))>(
                  annotations_of(^^Sizes<char, int>)[1]) == sizeof(int));
struct Structural { int value; };
template<Structural... Objects> struct [[=Objects...]] ObjectValues {};
static_assert(extract<Structural>(
                  annotations_of(^^ObjectValues<Structural{1}, Structural{2}>)[0])
                  .value == 1);
static_assert(extract<Structural>(
                  annotations_of(^^ObjectValues<Structural{1}, Structural{2}>)[1])
                  .value == 2);

int main(int, char**) { return 0; }
