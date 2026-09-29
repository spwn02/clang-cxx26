// RUN: rm -rf %t
// RUN: mkdir -p %t
// RUN: split-file --leading-lines %s %t
//
// Prepare the BMIs. mod_a defines Base/Derived; mod_b and mod_b2 each import
// mod_a and are each the *first* to construct the inheriting constructor
// `Derived::Derived(int&)` (via `using Base::Base;`) in their own,
// independent compilation. Because Derived is owned by mod_a but the
// constructor is synthesized while compiling mod_b (mod_b2), the writer
// records it as an update to an imported class
// (ASTWriter::AddedCXXImplicitMember's isImportedDeclContext path) rather
// than a normal locally-added member.
// RUN: %clang_cc1 -std=c++20 -triple x86_64-unknown-linux-gnu -emit-module-interface -o %t/mod_a.pcm %t/mod_a.cppm
// RUN: %clang_cc1 -std=c++20 -triple x86_64-unknown-linux-gnu -emit-module-interface -o %t/mod_b.pcm %t/mod_b.cppm -fmodule-file=mod_a=%t/mod_a.pcm
// RUN: %clang_cc1 -std=c++20 -triple x86_64-unknown-linux-gnu -emit-module-interface -o %t/mod_b2.pcm %t/mod_b2.cppm -fmodule-file=mod_a=%t/mod_a.pcm
//
// A TU that imports only mod_a and mod_b, and is *not* the one that first
// synthesized the constructor, must still see it as a member of Derived once
// deserialized: ASTReader::finishPendingActions replays the writer's update
// record via ASTReader::PendingAddedClassMembers, and used to call only
// CXXRecordDecl::addedMember (class-metadata bookkeeping) without ever
// splicing the constructor into Derived's lexical decl chain, so it was
// deserializable and reachable (e.g. via the CXXConstructExpr in mod_b that
// uses it) but absent from Derived->decls() -- invisible to any traversal
// that walks decls() directly, such as ParentMapContext's (PR144).
// RUN: %clang_cc1 -std=c++20 -triple x86_64-unknown-linux-gnu -ast-dump-all -ast-dump-filter Derived %t/test-single.cpp -fmodule-file=mod_a=%t/mod_a.pcm -fmodule-file=mod_b=%t/mod_b.pcm 2>&1 | FileCheck %s --check-prefix=SPLICED
//
// A TU that imports mod_a, mod_b, *and* mod_b2 replays two independent
// update records for what the reader's own redeclaration merging recognizes
// as the same constructor (one per module that synthesized it). The splice
// must not run twice: Derived must end up with exactly one instance of the
// constructor in its decls(), not two.
// RUN: %clang_cc1 -std=c++20 -triple x86_64-unknown-linux-gnu -ast-dump-all -ast-dump-filter Derived %t/test-dup.cpp -fmodule-file=mod_a=%t/mod_a.pcm -fmodule-file=mod_b=%t/mod_b.pcm -fmodule-file=mod_b2=%t/mod_b2.pcm > %t/dup.ast 2>&1
// RUN: FileCheck %s --check-prefix=SPLICED --input-file %t/dup.ast
// RUN: grep -c "imported in mod_a hidden implicit used constexpr Base 'void (int &) noexcept(false)' inline" %t/dup.ast | FileCheck %s --check-prefix=NODUP

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

//--- mod_b2.cppm
export module mod_b2;
export import mod_a;
export int useB2(int &x) {
  Derived d(x);
  return d.Ref;
}

//--- test-single.cpp
import mod_a;
import mod_b;
int useC(int &x) {
  Derived d(x);
  return d.Ref;
}

//--- test-dup.cpp
import mod_a;
import mod_b;
import mod_b2;
int useCDup(int &x) {
  Derived d(x);
  return d.Ref;
}

// The dup case's copy carries an extra "also in mod_a" redeclaration note
// as its first child (the single-module case doesn't), so match loosely
// rather than requiring the exact same child order in both cases.
// SPLICED-LABEL: Dumping Derived:
// SPLICED: CXXConstructorDecl {{.*}} imported in mod_a hidden implicit used constexpr Base 'void (int &) noexcept(false)' inline
// SPLICED: CXXCtorInitializer 'Base'
// SPLICED: CXXInheritedCtorInitExpr {{.*}} 'Base'

// NODUP: 1
