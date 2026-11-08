// RUN: %clang_cc1 -std=c++20 -fsyntax-only -verify=cxx20 %s
// RUN: %clang_cc1 -std=c++23 -fsyntax-only -verify=cxx20 %s
// RUN: %clang_cc1 -std=c++17 -fsyntax-only -verify=cxx17 %s
// RUN: %clang_cc1 -std=c++14 -fsyntax-only -verify=cxx17 %s
// RUN: %clang_cc1 -std=c++11 -fsyntax-only -verify=cxx17 %s

// A misspelled 'consteval' or 'constinit' is not corrected to the different keyword 'constexpr'.

constevla int f() { return 1; }   // cxx20-error {{unknown type name 'constevla'; did you mean 'consteval'?}} \
                                  // cxx17-error {{unknown type name 'constevla'; did you mean 'constexpr'?}}
constinti int g = 1;              // cxx20-error {{unknown type name 'constinti'; did you mean 'constinit'?}} \
                                  // cxx17-error {{unknown type name 'constinti'}}
template <class T> constevla int h(T) { return 1; } // cxx20-error {{unknown type name 'constevla'; did you mean 'consteval'?}} \
                                                     // cxx17-error {{unknown type name 'constevla'; did you mean 'constexpr'?}}
