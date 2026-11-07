// RUN: %clang_cc1 -std=c++2d -E %s | FileCheck %s --check-prefix=CXX29
// RUN: %clang_cc1 -std=c++2c -E %s | FileCheck %s --check-prefix=CXX26
// RUN: %clang_cc1 -std=c++23 -E %s | FileCheck %s --check-prefix=CXX26

// Pattern matching (P2688) is part of C++29 and exists in no earlier mode.

#if __has_feature(pattern_matching)
pattern_matching
#else
no_pattern_matching
#endif

// CXX29: pattern_matching
// CXX26: no_pattern_matching
