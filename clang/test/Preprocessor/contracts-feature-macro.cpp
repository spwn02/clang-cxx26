// RUN: %clang_cc1 -std=c++26 -E -dM %s | FileCheck %s --check-prefix=ENABLED
// RUN: %clang_cc1 -std=c++2d -E -dM %s | FileCheck %s --check-prefix=CXX29
// RUN: %clang_cc1 -std=c++23 -E -dM %s | FileCheck %s --check-prefix=DISABLED
// ENABLED: #define __cpp_contracts 202502L
// CXX29: #define __cpp_contracts 202606L
// DISABLED-NOT: #define __cpp_contracts
// [cpp.predefined]: __cpp_contracts is defined when contract assertions are supported.
