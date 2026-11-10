// -fexperimental-library is passed to CC1 and nothing is linked for it: the
// library facilities it enables are header-only for now.

// RUN: %clangxx -fexperimental-library -stdlib=libc++ -### %s 2>&1 | FileCheck %s
// RUN: %clangxx -fexperimental-library -stdlib=libstdc++ -### %s 2>&1 | FileCheck %s
// RUN: %clangxx -fexperimental-library -stdlib=libc++ -nostdlib++ -### %s 2>&1 | FileCheck %s

// CHECK: -fexperimental-library
// CHECK-NOT: -lc++experimental
