// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++26 -freflection -fexpansion-statements -emit-llvm -O2 -o - %s | FileCheck %s

// Expansion statements are equivalent to compound statements [stmt.expand].
// Their DeclContexts must not introduce a lambda capture boundary.
constexpr int enumerating() {
  auto f = [](auto v) {
    int n = 0;
    template for (auto x : {v, v}) { n += x; }
    return n;
  };
  return f(1);
}


struct Pair { int first, second; };
constexpr int destructuring() {
  auto f = [](auto const &t) {
    int n = 0;
    template for (auto x : t) { n += x; }
    return n;
  };
  return f(Pair{1, 2});
}


struct Range {
  int values[2];
  constexpr const int *begin() const { return values; }
  constexpr const int *end() const { return values + 2; }
};
constexpr int constexpr_initializers() {
  auto f = [](auto v) {
    constexpr int a = 1, b = 2;
    constexpr Pair p{a, b};
    constexpr Range r{{a, b}};
    int n = v;
    template for (constexpr auto x : {a, b}) { n += x; }
    template for (constexpr auto x : p) { n += x; }
    template for (constexpr auto x : r) { n += x; }
    return n;
  };
  return f(0);
}


constexpr int captures() {
  int n = 0;
  auto by_ref = [&](auto v) {
    template for (auto x : {v, v}) { n += x; }
    return n;
  };
  if (by_ref(1) != 2 || n != 2) return -1;
  auto explicit_copy = [n](auto v) mutable {
    template for (auto x : {v, v}) { n += x; }
    return n;
  };
  auto default_copy = [=](auto v) mutable {
    template for (auto x : {v, v}) { n += x; }
    return n;
  };
  return explicit_copy(1) + default_copy(2) + n;
}


constexpr int nested() {
  auto f = [](auto v) {
    int n = 0;
    template for (auto x : {v, v}) {
      template for (auto y : {x, x}) {
        n += y;
      }
      auto add = [&] { n += x; };
      add();
      auto copy = [x](auto z) { return x + z; };
      n += copy(0);
    }
    return n;
  };
  return f(1);
}


template<class T> constexpr int in_function_template(T v) {
  auto f = [](auto v) {
    int n = 0;
    template for (auto x : {v, v}) { n += x; }
    return n;
  };
  auto non_generic = [v] {
    int n = 0;
    template for (auto x : {v, v}) { n += x; }
    return n;
  };
  return f(v) + non_generic();
}


// CHECK-LABEL: define{{.*}} i32 @test_enumerating()
// CHECK: ret i32 2
extern "C" int test_enumerating() { return enumerating(); }

// CHECK-LABEL: define{{.*}} i32 @test_destructuring()
// CHECK: ret i32 3
extern "C" int test_destructuring() { return destructuring(); }

// CHECK-LABEL: define{{.*}} i32 @test_constexpr_initializers()
// CHECK: ret i32 9
extern "C" int test_constexpr_initializers() { return constexpr_initializers(); }

// CHECK-LABEL: define{{.*}} i32 @test_captures()
// CHECK: ret i32 12
extern "C" int test_captures() { return captures(); }

// CHECK-LABEL: define{{.*}} i32 @test_nested()
// CHECK: ret i32 8
extern "C" int test_nested() { return nested(); }

// CHECK-LABEL: define{{.*}} i32 @test_function_template()
// CHECK: ret i32 4
extern "C" int test_function_template() { return in_function_template(1); }

int main() {
  return test_enumerating() != 2 || test_destructuring() != 3 ||
         test_constexpr_initializers() != 9 || test_captures() != 12 ||
         test_nested() != 8 || test_function_template() != 4;
}
