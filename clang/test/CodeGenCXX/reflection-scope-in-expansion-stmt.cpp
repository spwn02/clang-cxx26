// RUN: %clang_cc1 -std=c++26 -freflection -fexpansion-statements -triple x86_64-unknown-linux-gnu -emit-llvm -o - %s | FileCheck %s

// The scope of an evaluation point inside an expansion statement is the enclosing
// function: the body of the statement is a declaration context, not an entity.
// A reflection of that declaration context used to reach the mangler and the
// linkage computation as the template argument of an instantiation.

using info = decltype(^^int);

[[clang::instantiation_dependent]] consteval info scope() { return __metafunction(122); }
[[clang::instantiation_dependent]] consteval info function_scope() { return __metafunction(123); }

struct Context {
  info scope;
};

template <Context C>
inline constexpr int v = 1;

// CHECK-LABEL: define {{.*}} @_Z1fv(
void f() {
  template for (constexpr int i : {0, 1}) {
    constexpr Context c{scope()};
    static_assert(c.scope == ^^f);
    static_assert(function_scope() == ^^f);
    (void)v<c>;
    (void)i;
  }
}

// CHECK-LABEL: define {{.*}} @_Z1gIiEvv(
template <class T>
void g() {
  template for (constexpr int i : {0, 1, 2}) {
    constexpr Context c{scope()};
    static_assert(c.scope == ^^g<T>);
    (void)v<c>;
    (void)i;
  }
}
template void g<int>();

struct S {
  void m() {
    template for (constexpr int i : {0}) {
      constexpr Context c{scope()};
      static_assert(c.scope == ^^m);
      (void)v<c>;
      (void)i;
    }
  }
};
// CHECK-LABEL: define {{.*}} @_Z3usev(
// CHECK-LABEL: define {{.*}} @_ZN1S1mEv(
void use() { S().m(); }
