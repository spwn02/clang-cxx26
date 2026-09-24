// RUN: %clang_cc1 -std=c++23 -emit-pch -o %t %s
// RUN: %clang_cc1 -std=c++23 -include-pch %t -verify %s
// expected-no-diagnostics

#ifndef HEADER
#define HEADER

template <typename T> struct B { B(T); };
template <typename T> struct C : B<T> { using B<T>::B; };

// Declares the guides of C, including the ones it inherits.
C header_use(1);

#else

// Guides declared after loading the PCH are inherited as well.
B(int) -> B<char>;
C c1(42);
static_assert(__is_same(decltype(c1), C<char>));
C c2(1.5);
static_assert(__is_same(decltype(c2), C<double>));

#endif
