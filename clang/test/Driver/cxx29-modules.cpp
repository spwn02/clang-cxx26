// C++29 builds on C++26: named modules, the module-file arguments and the C++26
// driver defaults (contract evaluation semantic, no blocks) apply to it too.

// RUN: rm -rf %t && mkdir %t && split-file %s %t

// The driver recognises the standard as one with C++20 modules.
// RUN: %clang -std=c++29 -fmodule-file=M=%t/M.pcm -fsyntax-only -### %t/u.cpp 2>&1 | FileCheck %s --check-prefix=MODS
// RUN: %clang -std=c++2d -fmodule-file=M=%t/M.pcm -fsyntax-only -### %t/u.cpp 2>&1 | FileCheck %s --check-prefix=MODS
// RUN: %clang -std=c++26 -fmodule-file=M=%t/M.pcm -fsyntax-only -### %t/u.cpp 2>&1 | FileCheck %s --check-prefix=MODS
// MODS: "-fno-implicit-modules"
// MODS: "-fmodule-file=M={{.*}}M.pcm"

// And it works end to end.
// RUN: %clang -std=c++29 --precompile %t/m.cppm -o %t/M.pcm
// RUN: %clang -std=c++29 -fmodule-file=M=%t/M.pcm -fsyntax-only %t/u.cpp
// RUN: %clang -std=c++2d -fmodule-file=M=%t/M.pcm -fsyntax-only %t/u.cpp

// Options that are forwarded for C++26 are forwarded for C++29.
// RUN: %clang -std=c++29 -fcontract-evaluation-semantic=enforce -fsyntax-only -### -x c++ %s 2>&1 | FileCheck %s --check-prefix=CONTRACTS
// CONTRACTS: "-fcontract-evaluation-semantic=enforce"

//--- m.cppm
export module M;
export int f() { return 1; }
//--- u.cpp
import M;
int main() { return f(); }
