// RUN: %clang_cc1 -std=c++2c -fsyntax-only -verify=cxx26 %s
// RUN: %clang_cc1 -std=c++23 -fsyntax-only -verify=old %s
// RUN: %clang_cc1 -std=c++17 -fsyntax-only -verify=old %s
// RUN: %clang_cc1 -std=c++11 -fsyntax-only -verify=old %s
// RUN: %clang_cc1 -x c -std=c23 -fsyntax-only -verify=old %s

// __builtin_start_lifetime (the support of std::start_lifetime, P3726R2) only exists in C++26 and later: earlier
// standards do not see it (and typo correction does not offer it for the name of another builtin).

#if __has_builtin(__builtin_start_lifetime)
#  define HAS 1
#else
#  define HAS 0
#endif

#if HAS
// old-no-diagnostics
#endif

struct A {
  int a;
};

void f(A* p) {
  __builtin_start_lifetime(p);
  // old-error@-1 {{use of undeclared identifier '__builtin_start_lifetime'}}
}

#if __cplusplus >= 202400L
static_assert(HAS == 1);
#else
static_assert(HAS == 0, "");
#endif
