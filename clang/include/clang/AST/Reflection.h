//===--- Reflection.h - Classes for representing reflection -----*- C++ -*-===//
//
// Copyright 2024 Bloomberg Finance L.P.
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_AST_REFLECTION_H
#define LLVM_CLANG_AST_REFLECTION_H

#include "clang/AST/ReflectionValue.h"
#include "clang/Basic/OperatorKinds.h"
#include "clang/AST/TypeBase.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/FoldingSet.h"
#include "llvm/ADT/SmallVector.h"
#include <optional>
#include <string>

namespace clang {

class APValue;
class CXXBaseSpecifier;

class Token;

/// \\brief Representation of a captured token sequence from ^^{ ... }.
/// Essentially an ArrayRef<Token> allocated in the ASTContext.
struct TokenSequenceData : public ArrayRef<Token> {
  TokenSequenceData() = default;
  TokenSequenceData(const Token *Data, size_t Length)
      : ArrayRef<Token>(Data, Length) {}
};

/// Allocate token sequence storage in the ASTContext. Empty sequences are
/// represented with an empty array.
/// std::meta::operators enumerates the overloadable operators in a fixed
/// order, with 0 meaning "no operator". These convert between that order and
/// OverloadedOperatorKind.
OverloadedOperatorKind getOverloadedOperatorForMetaIndex(unsigned Index);
unsigned getMetaIndexForOverloadedOperator(OverloadedOperatorKind OO);

TokenSequenceData CreateTokenSequenceData(ASTContext &Ctx,
                                          ArrayRef<Token> Tokens);

TokenSequenceData CreateTokenSequenceData(ASTContext &Ctx,
                                          ArrayRef<Token> Tokens1,
                                          ArrayRef<Token> Tokens2);

/// Allocate an empty token sequence in the ASTContext.
inline TokenSequenceData CreateEmptyTokenSequenceData(ASTContext &Ctx) {
  return CreateTokenSequenceData(Ctx, {});
}

/// Description of a hypothetical data member used by `define_class`.
struct TagDataMemberSpec {
  QualType Ty;
  std::optional<std::string> Name;
  std::optional<size_t> Alignment;
  std::optional<size_t> BitWidth;
  bool NoUniqueAddress;
  llvm::SmallVector<APValue *, 2> Annotations;

  bool operator==(const TagDataMemberSpec &Rhs) const;
  bool operator!=(const TagDataMemberSpec &Rhs) const;
};

} // namespace clang

#endif
