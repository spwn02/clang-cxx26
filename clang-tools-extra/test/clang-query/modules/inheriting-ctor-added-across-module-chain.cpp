// Regression test for PR144: an assertion-enabled clang-tidy/clang-query
// crashed ("Tried to match orphan node", UNREACHABLE at
// ASTMatchFinder.cpp) running a `hasAncestor` matcher over a TU that
// transitively imports a module chain in which an inheriting constructor
// was added to an imported class by a *different* module than the one that
// defines it. ParentMapContext.cpp gained a fallback (falling back to the
// declaration context when the parent map has no entry for an implicit,
// non-TU Decl) that keeps this matcher from crashing; this test exercises
// that fallback end-to-end through clang-query rather than only checking
// the AST shape directly (see clang/test/Modules/
// cxx-added-implicit-member-across-modules.cpp for that narrower check,
// which also verifies the underlying ASTReader fix that this fallback is
// defense-in-depth for).
//
// mod_b is the *first* to construct `Derived`, so it's the one whose own
// compilation synthesizes the inheriting constructor `Derived::Derived(int&)`
// (via `using Base::Base;`) for a class it doesn't own (Derived comes from
// mod_a); test-crash.cpp then imports both, transitively re-importing mod_a,
// and runs the matcher -- reproducing the real crash's shape (found
// verifying issue #121/#137 against real Switch/Miracle sources using
// std::optional<std::reference_wrapper<T>>) without needing the standard
// library.
//
// RUN: rm -rf %t
// RUN: mkdir -p %t
// RUN: split-file --leading-lines %s %t
// RUN: clang -std=c++20 --precompile %t/mod_a.cppm -o %t/mod_a.pcm
// RUN: clang -std=c++20 --precompile %t/mod_b.cppm -o %t/mod_b.pcm -fmodule-file=mod_a=%t/mod_a.pcm
//
// The crash case: TU imports mod_a and mod_b, is not the module that first
// synthesized the constructor, and runs a hasAncestor matcher over it.
// RUN: clang-query -c "match cxxConstructExpr(hasDeclaration(cxxConstructorDecl(hasAncestor(cxxRecordDecl()))))" %t/test-crash.cpp -- -std=c++20 --target=x86_64-unknown-linux-gnu -fmodule-file=mod_a=%t/mod_a.pcm -fmodule-file=mod_b=%t/mod_b.pcm | FileCheck %s
//
// Negative control: the constructor is synthesized and used in the same TU
// (no cross-module replay), so it was never affected by this bug.
// RUN: clang-query -c "match cxxConstructExpr(hasDeclaration(cxxConstructorDecl(hasAncestor(cxxRecordDecl()))))" %t/test-control-singletu.cpp -- -std=c++20 --target=x86_64-unknown-linux-gnu -fmodule-file=mod_a=%t/mod_a.pcm | FileCheck %s --check-prefix=CONTROL

//--- mod_a.cppm
export module mod_a;
export struct Base {
  constexpr Base(int &r) : Ref(r) {}
  int &Ref;
};
export struct Derived : Base {
  using Base::Base;
};

//--- mod_b.cppm
export module mod_b;
export import mod_a;
export int useB(int &x) {
  Derived d(x);
  return d.Ref;
}

//--- test-crash.cpp
import mod_a;
import mod_b;
int useC(int &x) {
  Derived d(x);
  return d.Ref;
}

//--- test-control-singletu.cpp
import mod_a;
int useSingle(int &x) {
  Derived d(x);
  return d.Ref;
}

// CHECK: match
// CHECK-NOT: Tried to match orphan node
// CHECK-NOT: UNREACHABLE

// CONTROL: 1 match.
