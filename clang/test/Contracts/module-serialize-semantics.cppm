// RUN: rm -rf %t
// RUN: split-file %s %t
// RUN: cd %t
//
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=observe -fcontract-group-evaluation-semantic=g=enforce %t/M.cppm -emit-module-interface -o %t/M.pcm
// RUN: %clang_cc1 -std=c++26 -fcontracts %t/M.pcm -emit-llvm -o - | FileCheck %s

// Code generated from a precompiled module interface must use the contract
// evaluation semantics the interface was precompiled with (#218), including
// the group a contract assertion belongs to: a deserialized ContractStmt keeps
// its contract_group attribute.

//--- M.cppm
export module M;
export void plain(int x) { contract_assert(x > 0); }
export void grouped(int x) { contract_assert [[clang::contract_group("g")]] (x > 0); }

// observe is 2, enforce is 3 ([contracts.syn] evaluation_semantic).
// CHECK-LABEL: define {{.*}} @_ZW1M5plaini
// CHECK: call void @__handle_contract_violation_v3(i32 {{(noundef )?}}2,
// CHECK-LABEL: define {{.*}} @_ZW1M7groupedi
// CHECK: call void @__handle_contract_violation_v3(i32 {{(noundef )?}}3,
