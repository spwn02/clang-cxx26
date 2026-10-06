// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++20 -emit-llvm %s -o - -disable-llvm-passes | FileCheck %s
// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++2c -emit-llvm %s -o - -disable-llvm-passes | FileCheck %s
#include "Inputs/coroutine.h"

// P3950R1: a promise type may declare both return_void and return_value; flowing off the end calls return_void.
struct task {
  struct promise_type {
    task get_return_object();
    std::suspend_never initial_suspend() noexcept;
    std::suspend_never final_suspend() noexcept;
    void unhandled_exception();
    void return_void();
    void return_value(int);
  };
};

// CHECK-LABEL: define {{.*}} @_Z9flows_offv(
// CHECK: call void @_ZN4task12promise_type11return_voidEv(
// CHECK-NOT: return_value
// CHECK: }
task flows_off() { co_await std::suspend_never{}; }

// CHECK-LABEL: define {{.*}} @_Z7a_valuev(
// CHECK: call void @_ZN4task12promise_type12return_valueEi(
// CHECK-NOT: return_void
// CHECK: }
task a_value() { co_return 3; }

// CHECK-LABEL: define {{.*}} @_Z6a_voidv(
// CHECK: call void @_ZN4task12promise_type11return_voidEv(
// CHECK: }
task a_void() { co_return; }
