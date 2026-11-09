// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify=cxx26 -pedantic -Wno-unused-const-variable --embed-dir=%S/Inputs %s
// RUN: %clang_cc1 -std=c++23 -fsyntax-only -verify=cxx23 --embed-dir=%S/Inputs %s
// RUN: %clang_cc1 -std=c++2d -fsyntax-only -verify=cxx26 -pedantic -Wno-unused-const-variable --embed-dir=%S/Inputs %s

// #embed is part of C++26 (P1967R14): no extension warning there, and __cpp_pp_embed is 202502L. Earlier modes keep the
// extension warning.
// cxx26-no-diagnostics

#if __cplusplus > 202302L
// The draft N5054 (P3540R3, C++29) has 202606L.
#  if __cplusplus > 202400L
#    if __cpp_pp_embed != 202606L
#      error "wrong value for __cpp_pp_embed"
#    endif
#  elif __cpp_pp_embed != 202502L
#    error "wrong value for __cpp_pp_embed"
#  endif
#elif defined(__cpp_pp_embed)
#  error "__cpp_pp_embed is only defined in C++26"
#endif

extern const char a[]; const char a[] = {
#embed "jk.txt" // cxx23-warning {{#embed is a Clang extension}}
};
static_assert(sizeof(a) == 2);
