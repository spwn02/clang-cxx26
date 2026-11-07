// Pattern matching survives a PCH round trip: each pattern matching code
// generation test is written to a PCH, read back by an empty translation unit
// that includes it, and the code generated there must satisfy the same checks
// as the code generated from the source. (A PCH is a prefix: the template
// instantiations happen in the consumer, from the deserialized patterns.)
//
// REQUIRES: x86-registered-target

// RUN: %clang_cc1 -triple x86_64-unknown-unknown -std=c++2d -fcxx-exceptions -O0 -emit-pch -o %t.1.pch %S/../CodeGenCXX/PatternMatching/match-and-pattern.cpp
// RUN: %clang_cc1 -triple x86_64-unknown-unknown -std=c++2d -fcxx-exceptions -O0 -include-pch %t.1.pch -x c++ /dev/null -emit-llvm -o - | FileCheck %S/../CodeGenCXX/PatternMatching/match-and-pattern.cpp
// RUN: %clang_cc1 -std=c++2d -O1 -disable-llvm-passes -emit-pch -o %t.2.pch %S/../CodeGenCXX/PatternMatching/match-case-attributes.cpp
// RUN: %clang_cc1 -std=c++2d -O1 -disable-llvm-passes -include-pch %t.2.pch -x c++ /dev/null -emit-llvm -o - | FileCheck %S/../CodeGenCXX/PatternMatching/match-case-attributes.cpp
// RUN: %clang_cc1 -triple x86_64-unknown-unknown -std=c++2d -O0 -emit-pch -o %t.3.pch %S/../CodeGenCXX/PatternMatching/match-empty-decomposition.cpp
// RUN: %clang_cc1 -triple x86_64-unknown-unknown -std=c++2d -O0 -include-pch %t.3.pch -x c++ /dev/null -emit-llvm -o - | FileCheck %S/../CodeGenCXX/PatternMatching/match-empty-decomposition.cpp
// RUN: %clang_cc1 -std=c++2d -triple x86_64-unknown-unknown -fcxx-exceptions -Wno-c++20-extensions -O0 -emit-pch -o %t.4.pch %S/../CodeGenCXX/PatternMatching/match-expr-agg.cpp
// RUN: %clang_cc1 -std=c++2d -triple x86_64-unknown-unknown -fcxx-exceptions -Wno-c++20-extensions -O0 -include-pch %t.4.pch -x c++ /dev/null -emit-llvm -o - | FileCheck %S/../CodeGenCXX/PatternMatching/match-expr-agg.cpp
// RUN: %clang_cc1 -triple x86_64-unknown-unknown -std=c++2d -fcxx-exceptions -O0 -emit-pch -o %t.5.pch %S/../CodeGenCXX/PatternMatching/match-expr-decomposition.cpp
// RUN: %clang_cc1 -triple x86_64-unknown-unknown -std=c++2d -fcxx-exceptions -O0 -include-pch %t.5.pch -x c++ /dev/null -emit-llvm -o - | FileCheck %S/../CodeGenCXX/PatternMatching/match-expr-decomposition.cpp
// RUN: %clang_cc1 -triple x86_64-unknown-unknown -std=c++2d -O0 -emit-pch -o %t.6.pch %S/../CodeGenCXX/PatternMatching/match-expr-pat-alternative-traits.cpp
// RUN: %clang_cc1 -triple x86_64-unknown-unknown -std=c++2d -O0 -include-pch %t.6.pch -x c++ /dev/null -emit-llvm -o - | grep '^define' | sed 's/ #[0-9]* .*//' | sort > %t.from-pch.txt
// RUN: %clang_cc1 -triple x86_64-unknown-unknown -std=c++2d -O0 -emit-llvm -o - %S/../CodeGenCXX/PatternMatching/match-expr-pat-alternative-traits.cpp | grep '^define' | sed 's/ #[0-9]* .*//' | sort > %t.direct.txt
// RUN: diff %t.direct.txt %t.from-pch.txt
// RUN: %clang_cc1 -triple x86_64-unknown-unknown -std=c++2d -O0 -emit-pch -o %t.7.pch %S/../CodeGenCXX/PatternMatching/match-expr-pat-declaration.cpp
// RUN: %clang_cc1 -triple x86_64-unknown-unknown -std=c++2d -O0 -include-pch %t.7.pch -x c++ /dev/null -emit-llvm -o - | FileCheck %S/../CodeGenCXX/PatternMatching/match-expr-pat-declaration.cpp
// RUN: %clang_cc1 -triple x86_64-unknown-unknown -std=c++2d -O0 -emit-pch -o %t.8.pch %S/../CodeGenCXX/PatternMatching/match-expr-pat-type.cpp
// RUN: %clang_cc1 -triple x86_64-unknown-unknown -std=c++2d -O0 -include-pch %t.8.pch -x c++ /dev/null -emit-llvm -o - | FileCheck %S/../CodeGenCXX/PatternMatching/match-expr-pat-type.cpp
// RUN: %clang_cc1 -std=c++2d -triple x86_64-unknown-unknown -O1 -emit-pch -o %t.9.pch %S/../CodeGenCXX/PatternMatching/match-or-pattern.cpp
// RUN: %clang_cc1 -std=c++2d -triple x86_64-unknown-unknown -O1 -include-pch %t.9.pch -x c++ /dev/null -emit-llvm -o - | FileCheck %S/../CodeGenCXX/PatternMatching/match-or-pattern.cpp
// RUN: %clang_cc1 -triple x86_64-unknown-unknown -std=c++2d -O0 -emit-pch -o %t.10.pch %S/../CodeGenCXX/PatternMatching/match-pattern-declaration.cpp
// RUN: %clang_cc1 -triple x86_64-unknown-unknown -std=c++2d -O0 -include-pch %t.10.pch -x c++ /dev/null -emit-llvm -o - | FileCheck %S/../CodeGenCXX/PatternMatching/match-pattern-declaration.cpp
// RUN: %clang_cc1 -std=c++2d -triple x86_64-unknown-unknown -O0 -emit-pch -o %t.11.pch %S/../CodeGenCXX/PatternMatching/match-select-expr.cpp
// RUN: %clang_cc1 -std=c++2d -triple x86_64-unknown-unknown -O0 -include-pch %t.11.pch -x c++ /dev/null -emit-llvm -o - | FileCheck %S/../CodeGenCXX/PatternMatching/match-select-expr.cpp
// RUN: %clang_cc1 -triple x86_64-unknown-unknown -std=c++2d -Werror=unused-value -O0 -emit-pch -o %t.12.pch %S/../CodeGenCXX/PatternMatching/match-test-condition-instantiation.cpp
// RUN: %clang_cc1 -triple x86_64-unknown-unknown -std=c++2d -Werror=unused-value -O0 -include-pch %t.12.pch -x c++ /dev/null -emit-llvm -o - | FileCheck %S/../CodeGenCXX/PatternMatching/match-test-condition-instantiation.cpp
// RUN: %clang_cc1 -triple x86_64-unknown-unknown -std=c++2d -O0 -emit-pch -o %t.13.pch %S/../CodeGenCXX/PatternMatching/match-test-expr-lifetime.cpp
// RUN: %clang_cc1 -triple x86_64-unknown-unknown -std=c++2d -O0 -include-pch %t.13.pch -x c++ /dev/null -emit-llvm -o - | FileCheck %S/../CodeGenCXX/PatternMatching/match-test-expr-lifetime.cpp
// RUN: %clang_cc1 -triple x86_64-unknown-unknown -std=c++2d -O0 -emit-pch -o %t.14.pch %S/../CodeGenCXX/PatternMatching/match-test-guard-move.cpp
// RUN: %clang_cc1 -triple x86_64-unknown-unknown -std=c++2d -O0 -include-pch %t.14.pch -x c++ /dev/null -emit-llvm -o - | grep '^define' | sed 's/ #[0-9]* .*//' | sort > %t.from-pch2.txt
// RUN: %clang_cc1 -triple x86_64-unknown-unknown -std=c++2d -O0 -emit-llvm -o - %S/../CodeGenCXX/PatternMatching/match-test-guard-move.cpp | grep '^define' | sed 's/ #[0-9]* .*//' | sort > %t.direct2.txt
// RUN: diff %t.direct2.txt %t.from-pch2.txt
// RUN: %clang_cc1 -std=c++2d -triple x86_64-unknown-unknown -O0 -emit-pch -o %t.15.pch %S/../CodeGenCXX/PatternMatching/match-value-switch.cpp
// RUN: %clang_cc1 -std=c++2d -triple x86_64-unknown-unknown -O0 -include-pch %t.15.pch -x c++ /dev/null -emit-llvm -o - | FileCheck %S/../CodeGenCXX/PatternMatching/match-value-switch.cpp
// RUN: %clang_cc1 -std=c++2d -triple x86_64-unknown-unknown -O1 -emit-pch -o %t.41.pch %S/../CodeGenCXX/PatternMatching/match-expr-pat-scalar.cpp
// RUN: %clang_cc1 -std=c++2d -triple x86_64-unknown-unknown -O1 -include-pch %t.41.pch -x c++ /dev/null -emit-llvm -o %t.41.ll
// RUN: FileCheck --input-file=%t.41.ll %S/../CodeGenCXX/PatternMatching/match-expr-pat-scalar.cpp
// RUN: %clang_cc1 -std=c++2d -triple x86_64-unknown-unknown -O0 -emit-pch -o %t.42.pch %S/../CodeGenCXX/PatternMatching/match-expr-pat-wildcard.cpp
// RUN: %clang_cc1 -std=c++2d -triple x86_64-unknown-unknown -O0 -include-pch %t.42.pch -x c++ /dev/null -emit-llvm -o %t.42.ll
// RUN: FileCheck --input-file=%t.42.ll %S/../CodeGenCXX/PatternMatching/match-expr-pat-wildcard.cpp
// RUN: %clang_cc1 -std=c++2d -triple x86_64-unknown-unknown -Wno-c++20-extensions -O0 -emit-pch -o %t.43.pch %S/../CodeGenCXX/PatternMatching/match-expr-stmt-action.cpp
// RUN: %clang_cc1 -std=c++2d -triple x86_64-unknown-unknown -Wno-c++20-extensions -O0 -include-pch %t.43.pch -x c++ /dev/null -emit-llvm -o - | grep '^define' | sed 's/ #[0-9]* .*//' | sort > %t.from-pch3.txt
// RUN: %clang_cc1 -std=c++2d -triple x86_64-unknown-unknown -Wno-c++20-extensions -O0 -emit-llvm -o - %S/../CodeGenCXX/PatternMatching/match-expr-stmt-action.cpp | grep '^define' | sed 's/ #[0-9]* .*//' | sort > %t.direct3.txt
// RUN: diff %t.direct3.txt %t.from-pch3.txt
// RUN: %clang_cc1 -std=c++2d -O0 -emit-pch -o %t.44.pch %S/../CodeGenCXX/cxx2d-match-predicate.cpp
// RUN: %clang_cc1 -std=c++2d -O0 -include-pch %t.44.pch -x c++ /dev/null -emit-llvm -o %t.44.ll
// RUN: FileCheck --input-file=%t.44.ll %S/../CodeGenCXX/cxx2d-match-predicate.cpp
// RUN: %clang_cc1 -std=c++2d -triple x86_64-unknown-linux-gnu -emit-pch -o %t.45.pch %S/../CodeGenCXX/cxx2d-nested-decomposition.cpp
// The order of the globals differs after a PCH, so compare what is defined.
// RUN: %clang_cc1 -std=c++2d -triple x86_64-unknown-linux-gnu -include-pch %t.45.pch -x c++ /dev/null -emit-llvm -o - | grep -E '^(@|define)' | sed -E 's/[0-9]{3,}/N/g; s/ #[0-9]+ .*//; s/, align.*//; s/_GLOBAL__sub_I_[A-Za-z0-9_.]*/_GLOBAL__sub_I/g' | sort > %t.from-pch4.txt
// RUN: %clang_cc1 -std=c++2d -triple x86_64-unknown-linux-gnu -emit-llvm -o - %S/../CodeGenCXX/cxx2d-nested-decomposition.cpp | grep -E '^(@|define)' | sed -E 's/[0-9]{3,}/N/g; s/ #[0-9]+ .*//; s/, align.*//; s/_GLOBAL__sub_I_[A-Za-z0-9_.]*/_GLOBAL__sub_I/g' | sort > %t.direct4.txt
// RUN: diff %t.direct4.txt %t.from-pch4.txt
