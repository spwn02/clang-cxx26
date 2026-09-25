// RUN: %clang_cc1 -std=c++26 -freflection -triple x86_64-unknown-linux-gnu -emit-llvm -o - %s | FileCheck %s

// A function template whose return type is written in terms of a dependent
// 'typename [: ... :]' splice has to be mangled while the spliced type is still
// unknown (its underlying type is the dependent placeholder). This used to hit
// llvm_unreachable("mangling a placeholder type") in the Itanium mangler.

using info = decltype(^^int);

template <class T> struct Box { T value; };

template <info R>
auto make() -> Box<typename [:R:]> { return {}; }

template <info R>
auto plain() -> typename [:R:] { return {}; }

// The splice operand is part of the mangled name: RT <expression> E.
// CHECK-DAG: define {{.*}}@_Z5plainIMt_ZTSiEERTT_Ev
// CHECK-DAG: define {{.*}}@_Z4makeIMt_ZTSiEE3BoxIRTT_EEv
int main() { (void)plain<^^int>(); return make<^^int>().value; }
