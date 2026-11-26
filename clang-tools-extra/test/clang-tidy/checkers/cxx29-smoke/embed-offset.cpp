// RUN: clang-tidy -checks='-*,modernize-*,readability-*,performance-*' %s -- -std=c++29 -Xclang --embed-dir=%S/../../../../../clang/test/Preprocessor/Inputs 2>&1 | FileCheck --allow-empty %s
// RUN: cp %s %t.cpp
// RUN: clang-tidy -checks='-*,modernize-*,readability-*,performance-*' -fix %t.cpp -- -std=c++29 -Xclang --embed-dir=%S/../../../../../clang/test/Preprocessor/Inputs
// RUN: clang++ -std=c++29 -fsyntax-only --embed-dir=%S/../../../../../clang/test/Preprocessor/Inputs %t.cpp
// CHECK-NOT: warning:
// CHECK-NOT: Assertion
// CHECK-NOT: Stack dump
// NOLINTBEGIN(readability-identifier-naming)

constexpr char bytes[] = { // NOLINT(modernize-avoid-c-arrays,readability-identifier-naming)
#embed <jk.txt> offset(1)
};
static_assert(sizeof(bytes) == 1);
static_assert(bytes[0] == 'k');

// NOLINTEND(readability-identifier-naming)
