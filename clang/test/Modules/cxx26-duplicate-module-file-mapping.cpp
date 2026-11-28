// RUN: rm -rf %t
// RUN: split-file %s %t
// RUN: %clang_cc1 -Wno-reserved-module-identifier -std=c++26 -emit-module-interface %t/std.cppm -o %t/std-cxx26.pcm
// RUN: %clang_cc1 -Wno-reserved-module-identifier -std=c++29 -emit-module-interface %t/std.cppm -o %t/std-cxx29.pcm
// RUN: %clang_cc1 -Wno-reserved-module-identifier -std=c++26 -fmodule-file=std=%t/std-cxx26.pcm -emit-module-interface %t/std.compat.cppm -o %t/std.compat-cxx26.pcm
// RUN: %clang_cc1 -Wno-reserved-module-identifier -std=c++29 -fmodule-file=std=%t/std-cxx29.pcm -emit-module-interface %t/std.compat.cppm -o %t/std.compat-cxx29.pcm
// RUN: not %clang_cc1 -std=c++29 -fsyntax-only %t/use.cpp \
// RUN:   -fmodule-file=std=%t/std-cxx26.pcm \
// RUN:   -fmodule-file=std.compat=%t/std.compat-cxx26.pcm \
// RUN:   -fmodule-file=std=%t/std-cxx29.pcm \
// RUN:   -fmodule-file=std.compat=%t/std.compat-cxx29.pcm 2>&1 | FileCheck %s

// CHECK: error: module 'std' is mapped to two different files: '{{.*}}std-cxx26.pcm' and '{{.*}}std-cxx29.pcm'
// CHECK: error: module 'std.compat' is mapped to two different files: '{{.*}}std.compat-cxx26.pcm' and '{{.*}}std.compat-cxx29.pcm'

//--- std.cppm
export module std;
export namespace std {
inline int module_value = 1;
struct S { static inline int static_member = 2; };
template <int N> inline int variable_template = N;
}

//--- std.compat.cppm
export module std.compat;
export import std;
export using std::module_value;
export using std::S;
export using std::variable_template;

//--- use.cpp
import std.compat;
