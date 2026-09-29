; RUN: opt -passes=attributor -S < %s | FileCheck %s
; RUN: opt -passes=function-attrs -S < %s | FileCheck %s --check-prefix=FA
;
; This file is distributed under the Apache License v2.0 with LLVM Exceptions.
; See https://llvm.org/LICENSE.txt for license information.
; SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

declare void @llvm.observable.checkpoint()
@CheckpointedValue = global i32 0

define void @checkpoint_does_not_willreturn() {
; CHECK-LABEL: define void @checkpoint_does_not_willreturn(
; CHECK-NOT: willreturn
; CHECK: call void @llvm.observable.checkpoint()
  call void @llvm.observable.checkpoint()
  ret void
}

define void @checkpoint_in_function() {
; CHECK-LABEL: define void @checkpoint_in_function(
; CHECK-NOT: willreturn
; CHECK: call void @llvm.observable.checkpoint()
  call void @llvm.observable.checkpoint()
  ret void
}

define void @checkpoint_prevents_return_inference() {
; CHECK-LABEL: define void @checkpoint_prevents_return_inference(
; CHECK-NOT: willreturn
; FA-LABEL: define void @checkpoint_prevents_return_inference(
; FA-SAME: )
; FA-NOT: willreturn
; FA: call void @llvm.observable.checkpoint()
  call void @llvm.observable.checkpoint()
  ret void
}

define void @checkpoint_blocks_readonly_mustprogress_inference() readonly mustprogress {
; CHECK-LABEL: define void @checkpoint_blocks_readonly_mustprogress_inference(
  call void @llvm.observable.checkpoint()
  ret void
}

define internal i32 @checkpointed_callee() noinline {
  call void @llvm.observable.checkpoint()
  %value = load i32, ptr @CheckpointedValue
  ret i32 %value
}

define void @checkpoint_blocks_callsite_inference() {
; CHECK-LABEL: define void @checkpoint_blocks_callsite_inference(
; CHECK-NOT: willreturn
; CHECK: %value = call i32 @checkpointed_callee() #{{[0-9]+}}
  %value = call i32 @checkpointed_callee() readonly mustprogress
  store i32 %value, ptr @CheckpointedValue
  ret void
}
; CHECK-NOT: willreturn
; FA-NOT: willreturn
