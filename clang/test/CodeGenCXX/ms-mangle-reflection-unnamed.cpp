// RUN: %clang_cc1 -triple x86_64-pc-windows-msvc -fms-compatibility -fms-extensions -std=c++2c -freflection -emit-llvm -o %t.ll %s
// RUN: FileCheck %s < %t.ll

// Members of different unnamed records, template parameters and template
// template parameters are distinct reflections and keep distinct Microsoft
// symbols (the Microsoft ABI's own unnamed-tag numbering does not distinguish
// the unnamed union from the unnamed struct); a collision is a redefinition
// error, so compiling and counting the definitions is the check.
using info = decltype(^^int);
template <info R> int code() { return 0; }
struct Anon {
  union { int a; float b; };
  struct { int c; int d; };
  union { int e; };
};
template <class T> int tp() { return code<^^T>(); }
template <template <class> class TT> int ttp() { return code<^^TT>(); }
template <class> struct Box {};
int an1 = code<^^Anon::a>();
int an2 = code<^^Anon::b>();
int an3 = code<^^Anon::c>();
int an4 = code<^^Anon::d>();
int an5 = code<^^Anon::e>();
int use = tp<int>() + tp<long>() + tp<Box<int>>() + ttp<Box>();
// CHECK-COUNT-9: define {{.*}}??$code@
