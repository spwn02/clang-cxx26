// RUN: %clang_cc1 -std=c++26 -freflection -fannotation-attributes -verify %s
// RUN: %clang_cc1 -std=c++26 -freflection -fannotation-attributes -emit-pch -o %t %s
// RUN: %clang_cc1 -std=c++26 -freflection -fannotation-attributes -include-pch %t -DRESTORE -verify %s
// expected-no-diagnostics

#ifndef RESTORE

template<int... Is> struct [[=Is...]] Values {};
Values<> empty;
Values<1, 2, 3> values;

template<class... Ts> struct [[=sizeof(Ts)...]] Sizes {};
Sizes<> no_sizes;
Sizes<char, int> sizes;

template<int... Is> void function [[=Is...]] () {}
template void function<>();
template void function<1, 2>();

template<int... Is> struct Members {
  [[=Is...]] int member;
  [[=Is...]] static constexpr int variable = 0;
};
Members<1, 2> members;
static_assert(Members<1, 2>::variable == 0);

// The inner declaration must retain the outer expansion until its enclosing
// template is specialized, and must expand the inner pack independently.
template<int... Outer> struct Nested {
  template<int... Inner> struct [[=Outer..., =Inner...]] InnerClass {};
};
Nested<1, 2>::InnerClass<3, 4> nested;

struct Structural { int value; };
template<Structural... Objects> struct [[=Objects...]] ObjectValues {};
ObjectValues<Structural{1}, Structural{2}> objects;

#else
Values<4, 5> restored_values;
Sizes<long, double> restored_sizes;
Members<4, 5> restored_members;
Nested<5, 6>::InnerClass<7, 8> restored_nested;
ObjectValues<Structural{3}, Structural{4}> restored_objects;
#endif
