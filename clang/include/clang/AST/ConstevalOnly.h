//===--- ConstevalOnly.h - Immediate object classification -------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_AST_CONSTEVALONLY_H
#define LLVM_CLANG_AST_CONSTEVALONLY_H

namespace clang {
class APValue;
class ASTContext;
class QualType;
class Expr;

/// Whether an object's value has a consteval-only constituent value or a
/// constituent reference to an immediate object or function ([expr.const]).
/// This inspects values, including the active member of a union, not types.
bool isImmediateObject(const APValue &Value, QualType Type,
                       const ASTContext &Context);
/// Whether the expression denotes an immediate object or produces a value
/// with consteval-only constituents. Dependent/unknown values are not classified.
bool hasImmediateValue(const Expr *E, const ASTContext &Context);
} // namespace clang

#endif
