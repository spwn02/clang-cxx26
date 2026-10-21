// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify=cxx26 %s
// RUN: %clang_cc1 -std=c++23 -fsyntax-only -verify=cxx23 %s

// [dcl.init.general]: std::nullptr_t is const-default-constructible (C++26).
using NP = decltype(nullptr);
const NP p;                 // cxx23-error {{default initialization of an object of const type 'const NP' (aka 'const std::nullptr_t')}}
struct S { const NP n; };   // cxx23-note 1+ {{default constructor of 'S' is implicitly deleted because field 'n' of const-qualified type 'const NP' (aka 'const std::nullptr_t') would not be initialized}}
S s;                        // cxx23-error {{call to implicitly-deleted default constructor of 'S'}}
const S s2;                 // cxx23-error {{call to implicitly-deleted default constructor of 'const S'}}
const NP arr[2];            // cxx23-error {{default initialization of an object of const type 'const NP[2]'}}
const int i;                // cxx26-error {{default initialization of an object of const type 'const int'}} cxx23-error {{default initialization of an object of const type 'const int'}}
