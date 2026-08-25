//===--- SpliceSpecifier.cpp - C++26 splice specifier ------------*- C++ -*-===//
//
// Copyright 2025 Bloomberg Finance L.P.
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "clang/AST/SpliceSpecifier.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Expr.h"
#include "clang/AST/TemplateBase.h"

namespace clang {

void *SpliceSpecifier::operator new(size_t Bytes, const ASTContext &C,
                                    unsigned Alignment) {
  return ::operator new(Bytes, C, Alignment);
}

SpliceSpecifier::SpliceSpecifier(
    SourceLocation LSpliceLoc, Expr *Operand, SourceLocation RSpliceLoc,
    const ASTTemplateArgumentListInfo *TemplateArgs)
    : LSpliceLoc(LSpliceLoc), Operand(Operand), RSpliceLoc(RSpliceLoc),
      TemplateArgs(TemplateArgs) {}

SpliceSpecifier *SpliceSpecifier::Create(
    ASTContext &C, SourceLocation LSpliceLoc, Expr *Operand,
    SourceLocation RSpliceLoc,
    const ASTTemplateArgumentListInfo *TemplateArgs) {
  return new (C) SpliceSpecifier(LSpliceLoc, Operand, RSpliceLoc, TemplateArgs);
}

SpliceSpecifierDependence SpliceSpecifier::getDependence() const {
  auto Result = toSpliceSpecifierDependence(Operand->getDependence());
  if (TemplateArgs)
    for (const auto &Arg : TemplateArgs->arguments())
      Result |= toSpliceSpecifierDependence(Arg.getArgument().getDependence());
  return Result;
}

SourceLocation SpliceSpecifier::getLAngleLoc() const {
  assert(isSpecialization());
  return TemplateArgs->getLAngleLoc();
}

SourceLocation SpliceSpecifier::getRAngleLoc() const {
  assert(isSpecialization());
  return TemplateArgs->getRAngleLoc();
}

} // namespace clang
