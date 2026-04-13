//===--- Reflection.cpp - Classes for representing reflection ---*- C++ -*-===//
//
// Copyright 2024 Bloomberg Finance L.P.
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "clang/AST/APValue.h"
#include "clang/AST/Reflection.h"
#include "llvm/ADT/FoldingSet.h"
#include "clang/AST/ASTContext.h"
#include "clang/Lex/Token.h"

namespace clang {

TokenSequenceData CreateTokenSequenceData(ASTContext &Ctx,
                                          ArrayRef<Token> Tokens) {
  Token *StoredTokens = nullptr;
  if (!Tokens.empty()) {
    StoredTokens = new (Ctx) Token[Tokens.size()];
    std::copy(Tokens.begin(), Tokens.end(), StoredTokens);
  }

  return TokenSequenceData(StoredTokens, Tokens.size());
}

TokenSequenceData CreateTokenSequenceData(ASTContext &Ctx,
                                          ArrayRef<Token> Tokens1,
                                          ArrayRef<Token> Tokens2) {

  Token *StoredTokens = nullptr;
  size_t Size = Tokens1.size() + Tokens2.size();
  if (Size != 0) {
    StoredTokens = new (Ctx) Token[Size];
    Token *Next = std::copy(Tokens1.begin(), Tokens1.end(), StoredTokens);
    std::copy(Tokens2.begin(), Tokens2.end(), Next);
  }

  return TokenSequenceData(StoredTokens, Size);
}

bool TagDataMemberSpec::operator==(const TagDataMemberSpec &Rhs) const {
  if (!(Ty == Rhs.Ty && Alignment == Rhs.Alignment &&
        BitWidth == Rhs.BitWidth && Name == Rhs.Name &&
        NoUniqueAddress == Rhs.NoUniqueAddress &&
        Annotations.size() == Rhs.Annotations.size()))
    return false;
  for (size_t I = 0; I != Annotations.size(); ++I) {
    llvm::FoldingSetNodeID L, R;
    Annotations[I]->Profile(L);
    Rhs.Annotations[I]->Profile(R);
    if (L != R)
      return false;
  }
  return true;
}

bool TagDataMemberSpec::operator!=(const TagDataMemberSpec &Rhs) const {
  return !(*this == Rhs);
}

} // namespace clang
