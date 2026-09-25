// A consteval block is a member-like declaration of a class but has no access
// specifier, like a static_assert. Writing it to a PCH or module used to trip
// Decl::AccessDeclContextCheck ("Access specifier is AS_none inside a record
// decl") in assertion-enabled builds.
//
// RUN: %clang_cc1 -std=c++26 -freflection -emit-pch -o %t.pch %s
// RUN: %clang_cc1 -std=c++26 -freflection -include-pch %t.pch -verify %s
// expected-no-diagnostics

#ifndef HEADER
#define HEADER

struct Plain {
  consteval {}
  int x;
};

template <class T>
struct Templated {
  consteval {}
  T value;
};

inline Templated<int> instance{};

#else

static_assert(sizeof(Plain) == sizeof(int));
static_assert(sizeof(Templated<char>) == sizeof(char));

#endif
