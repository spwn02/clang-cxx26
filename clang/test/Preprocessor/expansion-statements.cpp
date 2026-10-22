// RUN: %clang_cc1 -std=c++26 -E -dM %s | FileCheck %s --check-prefix=ENABLED
// RUN: %clang_cc1 -std=c++23 -E -dM %s | FileCheck %s --check-prefix=DISABLED
// ENABLED: #define __cpp_expansion_statements 202506L
// DISABLED-NOT: #define __cpp_expansion_statements
