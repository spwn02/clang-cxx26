// A module file that is cut off (a compile killed halfway, a full disk) must be
// rejected with a diagnostic, never crash the reader (#488). The importer has
// misspelled names so typo correction (which iterates the module's
// identifiers) runs too.
//
// RUN: rm -rf %t && mkdir -p %t && split-file %s %t
// RUN: %clang -std=c++20 --precompile %t/m.cppm -o %t/m.pcm
// RUN: %python -c "import sys; d=open(sys.argv[1],'rb').read(); [open(sys.argv[2]+str(p)+'.pcm','wb').write(d[:len(d)*p//100]) for p in (10,30,60,90,99)]; open(sys.argv[2]+'tail.pcm','wb').write(d[:-3])" %t/m.pcm %t/t
// RUN: not %clang -std=c++20 -fmodule-file=M=%t/t10.pcm -fsyntax-only %t/use.cpp 2>&1 | FileCheck %s
// RUN: not %clang -std=c++20 -fmodule-file=M=%t/t30.pcm -fsyntax-only %t/use.cpp 2>&1 | FileCheck %s
// RUN: not %clang -std=c++20 -fmodule-file=M=%t/t60.pcm -fsyntax-only %t/use.cpp 2>&1 | FileCheck %s
// RUN: not %clang -std=c++20 -fmodule-file=M=%t/t90.pcm -fsyntax-only %t/use.cpp 2>&1 | FileCheck %s
// RUN: not %clang -std=c++20 -fmodule-file=M=%t/t99.pcm -fsyntax-only %t/use.cpp 2>&1 | FileCheck %s
// RUN: not %clang -std=c++20 -fmodule-file=M=%t/ttail.pcm -fsyntax-only %t/use.cpp 2>&1 | FileCheck %s
// CHECK: fatal error: malformed or corrupted precompiled file

//--- m.cppm
export module M;
export int answer() { return 42; }
export struct Widget { int size; int count; };

//--- use.cpp
import M;
int main() { return answr() + Widgit{1, 2}.size; }
