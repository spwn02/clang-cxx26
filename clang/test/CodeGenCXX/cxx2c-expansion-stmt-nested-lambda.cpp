// RUN: %clang_cc1 -std=c++26 -freflection -fexpansion-statements -O2 -emit-llvm -o - %s | FileCheck %s

// CHECK-LABEL: define {{.*}} i32 @_Z7genericv()
// CHECK: ret i32 4
int generic() {
  auto g = [](auto v) {
    int n = 0;
    template for (auto x : {v, v}) {
      template for (auto y : {x, x}) {
        auto add = [&] { n += y; };
        add();
      }
    }
    return n;
  };
  return g(1);
}

// CHECK-LABEL: define {{.*}} i32 @_Z11non_genericv()
// CHECK: ret i32 6
int non_generic() {
  int n = 0;
  template for (auto x : {1, 2}) {
    template for (auto y : {x, x}) {
      auto add = [&] { n += y; };
      add();
    }
  }
  return n;
}
