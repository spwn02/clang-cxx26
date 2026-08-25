//===- SpliceSpecifier.h - C++26 splice specifier ---------------*- C++ -*-===//
//
// Copyright 2025 Bloomberg Finance L.P.
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_AST_SPLICESPECIFIER_H
#define LLVM_CLANG_AST_SPLICESPECIFIER_H

#include "clang/AST/DependenceFlags.h"
#include "clang/Basic/SourceLocation.h"

namespace clang {

class ASTContext;
struct ASTTemplateArgumentListInfo;
class Expr;

/// Represents a C++26 splice-specifier and optional explicit template args.
class alignas(void *) SpliceSpecifier {
  SourceLocation LSpliceLoc;
  Expr *Operand;
  SourceLocation RSpliceLoc;
  const ASTTemplateArgumentListInfo *TemplateArgs;

  SpliceSpecifier(SourceLocation LSpliceLoc, Expr *Operand,
                  SourceLocation RSpliceLoc,
                  const ASTTemplateArgumentListInfo *TemplateArgs);

public:
  void *operator new(size_t Bytes, const ASTContext &C, unsigned Alignment = 8);
  void *operator new(size_t Bytes, const ASTContext *C, unsigned Alignment = 8) {
    return operator new(Bytes, *C, Alignment);
  }

  static SpliceSpecifier *Create(ASTContext &C, SourceLocation LSpliceLoc,
                                 Expr *Operand, SourceLocation RSpliceLoc,
                                 const ASTTemplateArgumentListInfo *TemplateArgs);

  Expr *getOperand() const { return Operand; }
  void setOperand(Expr *E) { Operand = E; }
  SpliceSpecifierDependence getDependence() const;
  SourceLocation getLSpliceLoc() const { return LSpliceLoc; }
  SourceLocation getRSpliceLoc() const { return RSpliceLoc; }
  SourceLocation getLAngleLoc() const;
  SourceLocation getRAngleLoc() const;
  SourceLocation getBeginLoc() const { return LSpliceLoc; }
  SourceLocation getEndLoc() const {
    return isSpecialization() ? getRAngleLoc() : RSpliceLoc;
  }
  SourceRange getSourceRange() const { return {getBeginLoc(), getEndLoc()}; }
  bool isSpecialization() const { return TemplateArgs != nullptr; }
  const ASTTemplateArgumentListInfo *getTemplateArgs() const {
    return TemplateArgs;
  }
};

} // namespace clang

#endif
