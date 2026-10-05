// RUN: %clang_cc1 -std=c++2c -fsyntax-only -verify %s -DEXPECT_BUILTIN=1
// RUN: %clang_cc1 -std=c++23 -fsyntax-only -verify %s -DEXPECT_BUILTIN=0
// RUN: %clang_cc1 -std=c++20 -fsyntax-only -verify %s -DEXPECT_BUILTIN=0
// RUN: %clang_cc1 -std=c++17 -fsyntax-only -verify %s -DEXPECT_BUILTIN=0
// RUN: %clang_cc1 -std=c++11 -fsyntax-only -verify %s -DEXPECT_BUILTIN=0
// RUN: %clang_cc1 -std=c++03 -fsyntax-only -verify %s -DEXPECT_BUILTIN=0
// RUN: %clang_cc1 -x c -std=c23 -fsyntax-only -verify %s -DEXPECT_BUILTIN=0

// expected-no-diagnostics

// __builtin_start_lifetime (the support of std::start_lifetime, P3726R2) only exists in C++26 and later.

#if __has_builtin(__builtin_start_lifetime)
#  define HAS 1
#else
#  define HAS 0
#endif

#if HAS != EXPECT_BUILTIN
#  error wrong availability of __builtin_start_lifetime
#endif
