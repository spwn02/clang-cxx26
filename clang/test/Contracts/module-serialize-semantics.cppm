// RUN: rm -rf %t
// RUN: split-file %s %t
// RUN: cd %t
//
// The interface compiled to an object (#218): code generated from a
// precompiled module interface uses the contract evaluation semantics it was
// precompiled with.
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=observe -fcontract-group-evaluation-semantic=g=enforce %t/M.cppm -emit-module-interface -o %t/M.pcm
// RUN: %clang_cc1 -std=c++26 -fcontracts %t/M.pcm -emit-llvm -o - | FileCheck %s --check-prefix=BMI
//
// An importer generating code for an inline function it imported uses its own
// flags; the deserialized assertion must still carry its contract_group
// attribute.
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=observe -fcontract-group-evaluation-semantic=g=enforce -fprebuilt-module-path=%t %t/Use.cpp -emit-llvm -o - | FileCheck %s --check-prefix=IMPORT
//
// The same through a precompiled header.
// RUN: %clang_cc1 -std=c++26 -fcontracts -x c++-header %t/h.h -emit-pch -o %t/h.pch
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=observe -fcontract-group-evaluation-semantic=g=enforce -include-pch %t/h.pch %t/UsePch.cpp -emit-llvm -o - | FileCheck %s --check-prefix=PCH

// A deserialized ContractStmt keeps its contract_group attribute, and the AST
// file records the semantics it was written with. observe is 2, enforce is 3
// ([contracts.syn] evaluation_semantic).

//--- M.cppm
export module M;
export void plain(int x) { contract_assert(x > 0); }
export void grouped(int x) { contract_assert [[clang::contract_group("g")]] (x > 0); }
export inline void h(int x) { contract_assert [[clang::contract_group("g")]] (x > 0); }

// BMI-LABEL: define {{.*}} @_ZW1M5plaini
// BMI: call void @__handle_contract_violation_v3(i32 {{(noundef )?}}2,
// BMI-LABEL: define {{.*}} @_ZW1M7groupedi
// BMI: call void @__handle_contract_violation_v3(i32 {{(noundef )?}}3,

//--- Use.cpp
import M;
void use() { h(1); }

// IMPORT-LABEL: define {{.*}} @_ZW1M1hi
// IMPORT: call void @__handle_contract_violation_v3(i32 {{(noundef )?}}3,

//--- h.h
inline void h(int x) { contract_assert [[clang::contract_group("g")]] (x > 0); }

//--- UsePch.cpp
void use() { h(1); }

// PCH-LABEL: define {{.*}} @_Z1hi
// PCH: call void @__handle_contract_violation_v3(i32 {{(noundef )?}}3,
