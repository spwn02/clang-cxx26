//===- ReflectionEvaluationContext.h - Injection visibility -------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_AST_REFLECTIONEVALUATIONCONTEXT_H
#define LLVM_CLANG_AST_REFLECTIONEVALUATIONCONTEXT_H

#include <cstdint>

namespace clang {
class ASTContext;
class Decl;
struct ReflectionEvaluationState;

/// A manifestly constant-evaluated expression starts its own evaluation
/// context ([expr.const.reflect]). Ordinary calls retain the active context.
class ReflectionEvaluationScope {
  ReflectionEvaluationState &State;
  ReflectionEvaluationScope *Previous;
  uint64_t Baseline;
  uint64_t FirstProduced;
  friend bool isReflectionDefinitionVisible(ASTContext &, const Decl *);

public:
  explicit ReflectionEvaluationScope(ASTContext &C);
  ~ReflectionEvaluationScope();
  ReflectionEvaluationScope(const ReflectionEvaluationScope &) = delete;
  ReflectionEvaluationScope &operator=(const ReflectionEvaluationScope &) = delete;
};

void recordReflectionInjection(ASTContext &C, const Decl *D);
bool isReflectionDefinitionVisible(ASTContext &C, const Decl *D);
} // namespace clang

#endif
