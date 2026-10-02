// RUN: %clang_cc1 -std=c++26 -freflection -fexpansion-statements -O2 -emit-llvm -o - %s | FileCheck %s

template <class T> int by_default_impl(T v) {
  int n = 0;
  template for (auto x : {v, v}) {
    template for (auto y : {x, x}) {
      auto add = [=, &n] { n += y; };
      add();
    }
  }
  return n;
}
// CHECK-LABEL: define {{.*}} i32 @_Z10by_defaultv()
// CHECK: ret i32 4
int by_default() { return by_default_impl(1); }

// CHECK-LABEL: define {{.*}} i32 @_Z20generic_three_levelsv()
// CHECK: ret i32 12
int generic_three_levels() {
  auto g = [](auto v) {
    int n = 0;
    template for (auto x : {v, v}) {
      template for (auto y : {x, x, x}) {
        template for (auto z : {y, y}) {
          auto add = [&n, z] { n += z; };
          add();
        }
      }
    }
    return n;
  };
  return g(1);
}

template <class T> int init_capture_impl(T v) {
  int n = 0;
  template for (auto x : {v, v}) {
    template for (auto y : {x, x}) {
      auto add = [w = y, &n] { n += w; };
      add();
    }
  }
  return n;
}
// CHECK-LABEL: define {{.*}} i32 @_Z12init_capturev()
// CHECK: ret i32 4
int init_capture() { return init_capture_impl(1); }

template <class T> int mutable_copy_impl(T v) {
  int n = 0;
  template for (auto x : {v, v}) {
    template for (auto y : {x, x}) {
      auto add = [y, &n]() mutable { n += y++; };
      add();
      n += y - 1; // Mutating the copy must leave the expansion variable intact.
    }
  }
  return n;
}
// CHECK-LABEL: define {{.*}} i32 @_Z12mutable_copyv()
// CHECK: ret i32 4
int mutable_copy() { return mutable_copy_impl(1); }

// CHECK-LABEL: define {{.*}} i32 @_Z12non_templatev()
// CHECK: ret i32 4
int non_template() {
  int n = 0;
  template for (auto x : {1, 1}) {
    template for (auto y : {x, x}) {
      auto add = [y, &n] { n += y; };
      add();
    }
  }
  return n;
}

struct Copyable {
  int value;
  int *copies;
  Copyable(int value, int &copies) : value(value), copies(&copies) {}
  Copyable(const Copyable &other) : value(other.value), copies(other.copies) {
    ++*copies;
  }
};
template <class T> int class_copy_impl(T v) {
  int n = 0;
  template for (auto x : {v, v}) {
    template for (auto y : {x, x}) {
      int before = *y.copies;
      auto add = [y] { return y.value; };
      if (*y.copies != before + 1)
        return -100;
      n += add();
    }
  }
  return n;
}
// CHECK-LABEL: define {{.*}} i32 @_Z10class_copyv()
// CHECK: ret i32 4
int class_copy() {
  int copies = 0;
  return class_copy_impl(Copyable(1, copies));
}

#ifdef RUN_RUNTIME_TEST
int main() {
  if (by_default() != 4)
    return 1;
  if (generic_three_levels() != 12)
    return 2;
  if (init_capture() != 4)
    return 3;
  if (mutable_copy() != 4)
    return 4;
  if (non_template() != 4)
    return 5;
  if (class_copy() != 4)
    return 6;
  return 0;
}
#endif
