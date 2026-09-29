; RUN: opt < %s -passes=instcombine -S | FileCheck %s
; RUN: opt < %s -passes=simplifycfg -S | FileCheck %s --check-prefix=CFG
; RUN: opt < %s -passes='default<O0>' -S | FileCheck %s --check-prefix=O0
; RUN: opt < %s -passes='default<O2>' -S | FileCheck %s
; RUN: opt < %s -passes='default<O3>' -S | FileCheck %s
; RUN: opt < %s -passes='thinlto-pre-link<O3>' -S | FileCheck %s --check-prefix=LTO
; RUN: opt < %s -passes='lto<O3>' -S | FileCheck %s --check-prefix=LTOFULL
;
; This file is distributed under the Apache License v2.0 with LLVM Exceptions.
; See https://llvm.org/LICENSE.txt for license information.
; SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

declare void @llvm.observable.checkpoint()
declare void @llvm.assume(i1)
declare void @observe() nounwind willreturn

define void @preserve_prefix() {
; O0-LABEL: @preserve_prefix(
; O0: call void @observe()
; O0: call void @llvm.observable.checkpoint()
; O0: load i32, ptr null
; CHECK-LABEL: @preserve_prefix(
; CHECK: call void @observe()
; CHECK: call void @llvm.observable.checkpoint()
; CFG-LABEL: @preserve_prefix(
; CFG: call void @observe()
; CFG: call void @llvm.observable.checkpoint()
; LTO-LABEL: @preserve_prefix(
; LTO: call void @observe()
; LTO: call void @llvm.observable.checkpoint()
; LTOFULL-LABEL: @preserve_prefix(
; LTOFULL: call void @observe()
; LTOFULL: call void @llvm.observable.checkpoint()
entry:
  call void @observe()
  call void @llvm.observable.checkpoint()
  %v = load i32, ptr null
  ret void
}

define void @without_checkpoint() {
; O0-LABEL: @without_checkpoint(
; O0: call void @observe()
; O0: unreachable
; CHECK-LABEL: @without_checkpoint(
; CHECK-NOT: call void @observe()
; CHECK: unreachable
; CFG-LABEL: @without_checkpoint(
; CFG-NOT: call void @observe()
; CFG: unreachable
; LTO-LABEL: @without_checkpoint(
; LTO-NOT: call void @observe()
; LTO: unreachable
; LTOFULL-LABEL: @without_checkpoint(
; LTOFULL-NOT: call void @observe()
; LTOFULL: unreachable
entry:
  call void @observe()
  unreachable
}

define void @unknown_indirect_call_before_ub(ptr %callee) {
; O0-LABEL: @unknown_indirect_call_before_ub(
; O0: call void %callee()
; O0: load i32, ptr null
; CHECK-LABEL: @unknown_indirect_call_before_ub(
; CHECK: call void %callee()
; CFG-LABEL: @unknown_indirect_call_before_ub(
; CFG: call void %callee()
; LTO-LABEL: @unknown_indirect_call_before_ub(
; LTO: call void %callee()
; LTOFULL-LABEL: @unknown_indirect_call_before_ub(
; LTOFULL: call void %callee()
entry:
  call void %callee()
  %v = load i32, ptr null
  ret void
}

define void @overflow_after_checkpoint() {
; O0-LABEL: @overflow_after_checkpoint(
; O0: call void @observe()
; O0: call void @llvm.observable.checkpoint()
; O0: add nsw i32 2147483647, 1
; CHECK-LABEL: @overflow_after_checkpoint(
; CHECK: call void @observe()
; CHECK: call void @llvm.observable.checkpoint()
; CFG-LABEL: @overflow_after_checkpoint(
; CFG: call void @observe()
; CFG: call void @llvm.observable.checkpoint()
; LTO-LABEL: @overflow_after_checkpoint(
; LTO: call void @observe()
; LTO: call void @llvm.observable.checkpoint()
; LTOFULL-LABEL: @overflow_after_checkpoint(
; LTOFULL: call void @observe()
; LTOFULL: call void @llvm.observable.checkpoint()
entry:
  call void @observe()
  call void @llvm.observable.checkpoint()
  %v = add nsw i32 2147483647, 1
  %condition = icmp eq i32 %v, 0
  call void @llvm.assume(i1 %condition)
  ret void
}

define void @direct_unreachable_after_checkpoint() {
; O0-LABEL: @direct_unreachable_after_checkpoint(
; O0: call void @observe()
; O0: call void @llvm.observable.checkpoint()
; O0: unreachable
; CHECK-LABEL: @direct_unreachable_after_checkpoint(
; CHECK: call void @observe()
; CHECK: call void @llvm.observable.checkpoint()
; CHECK: unreachable
; CFG-LABEL: @direct_unreachable_after_checkpoint(
; CFG: call void @observe()
; CFG: call void @llvm.observable.checkpoint()
; LTOFULL-LABEL: @direct_unreachable_after_checkpoint(
; LTOFULL: call void @observe()
; LTOFULL: call void @llvm.observable.checkpoint()
entry:
  call void @observe()
  call void @llvm.observable.checkpoint()
  unreachable
}
