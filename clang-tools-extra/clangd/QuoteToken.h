//===--- QuoteToken.h - Navigation for token-sequence quotes ----*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANGD_QUOTETOKEN_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANGD_QUOTETOKEN_H

#include "clang/Basic/SourceLocation.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallVector.h"

namespace clang {
class ASTContext;
class Decl;
class NamedDecl;
class SourceManager;
namespace clangd {

enum class QuoteTokenRole { Reference, Declaration };

/// Resolved identifier information for a token in a ^^{} quote.
class QuoteTokenTable {
public:
  struct Target {
    const NamedDecl *Decl;
    QuoteTokenRole Role;
  };
  struct Resolution {
    llvm::SmallVector<Target, 1> Targets;
    unsigned Expansions = 0;
    bool Ambiguous = false;
  };
  struct Reference {
    SourceLocation Loc;
    QuoteTokenRole Role;
  };

  /// Returns true when p Loc names a quote-body token, even if it has not
  /// expanded or its expansions disagree.
  bool contains(SourceLocation Loc, const SourceManager &SM) const;
  /// Returns the resolution only when all resolving expansions agree.
  const Resolution *lookup(SourceLocation Loc, const SourceManager &SM) const;
  SourceLocation tokenLocation(SourceLocation Loc,
                               const SourceManager &SM) const;
  llvm::SmallVector<Reference, 4> references(const NamedDecl *D) const;

  void addQuotedToken(SourceLocation Loc, unsigned Offset, unsigned Length) {
    Tokens.try_emplace(Offset, TokenInfo{Loc, Length, {}});
  }

  static QuoteTokenTable build(ASTContext &Ctx,
                               llvm::ArrayRef<Decl *> TopLevelDecls);

private:
  struct TokenInfo {
    SourceLocation Loc;
    unsigned Length;
    Resolution Result;
  };
  const TokenInfo *findToken(SourceLocation Loc,
                             const SourceManager &SM) const;
  llvm::DenseMap<unsigned, TokenInfo> Tokens;
};

} // namespace clangd
} // namespace clang

#endif
