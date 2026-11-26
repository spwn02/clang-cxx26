// RUN: rm -rf %t/26 && mkdir -p %t/26
// RUN: split-file %s %t/26
// RUN: %clang_cc1 -std=c++26 -x c++-header %t/26/h.h -emit-pch -o %t/26/h.pch
// RUN: %clang_cc1 -std=c++26 -include-pch %t/26/h.pch %t/26/use.cpp -ast-print | FileCheck %s
// RUN: %clang_cc1 -std=c++26 %t/26/M.cppm -emit-module-interface -o %t/26/M.pcm
// RUN: %clang_cc1 -std=c++26 -fmodule-file=M=%t/26/M.pcm %t/26/use-module.cpp -ast-dump-all | FileCheck %s --check-prefix=MODULE
// CHECK: pre<std::contracts::controls::enforced>(x > 0)
// MODULE: ContractStmt{{.*}}pre <std::contracts::controls::enforced>

//--- h.h
namespace std::contracts::controls {
inline namespace __p4451 {
struct __enforced_t {};
inline constexpr __enforced_t enforced{};
}
}
template <class T> T checked(T x) pre<std::contracts::controls::enforced>(x > 0) { return x; }

//--- use.cpp
int use() { return checked(1); }

//--- M.cppm
export module M;
export namespace std::contracts::controls {
inline namespace __p4451 {
struct __enforced_t {};
inline constexpr __enforced_t enforced{};
}
}
export template <class T> T checked(T x) pre<std::contracts::controls::enforced>(x > 0) { return x; }

//--- use-module.cpp
import M;
int use() { return checked(1); }
