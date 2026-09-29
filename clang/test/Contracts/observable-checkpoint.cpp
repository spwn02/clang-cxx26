// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=observe -emit-llvm -o - %s | FileCheck %s

// This file is distributed under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

void f(int value) pre(value != 0) {
}

// CHECK-LABEL: define{{.*}} @_Z1fi
// CHECK: call void @__handle_contract_violation_v3
// CHECK-NEXT: call void @llvm.observable.checkpoint()
// CHECK-NEXT: br label %contract.end
