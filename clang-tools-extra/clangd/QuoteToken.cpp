//===--- QuoteToken.cpp - Navigation for token-sequence quotes -----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "QuoteToken.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/ExprCXX.h"
#include "clang/AST/NestedNameSpecifier.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/TypeLoc.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Lex/Token.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"

namespace clang::clangd {
namespace {

class QuoteCollector : public RecursiveASTVisitor<QuoteCollector> {
public:
  QuoteCollector(const SourceManager &SM, QuoteTokenTable &Table)
      : SM(SM), Table(Table) {}

  bool TraverseFunctionDecl(FunctionDecl *FD) {
    bool WasExpressionMacro = InExpressionMacro;
    InExpressionMacro |= FD->isExpressionMacro();
    bool Result = RecursiveASTVisitor::TraverseFunctionDecl(FD);
    InExpressionMacro = WasExpressionMacro;
    return Result;
  }

  bool VisitCXXTokenSequenceExpr(CXXTokenSequenceExpr *E) {
    for (const Token &Tok : E->getValue().getTokenSequence()) {
      // Annotation tokens (e.g. \(x) holes) carry no length and no identifier.
      if (Tok.isAnnotation())
        continue;
      SourceLocation Loc = SM.getSpellingLoc(Tok.getLocation());
      if (Loc.isInvalid() || SM.getFileID(Loc) != SM.getMainFileID())
        continue;
      Table.addQuotedToken(Loc, SM.getFileOffset(Loc), Tok.getLength(),
                           InExpressionMacro);
    }
    return true;
  }

private:
  const SourceManager &SM;
  QuoteTokenTable &Table;
  bool InExpressionMacro = false;
};

struct ExpansionTargets {
  llvm::DenseMap<const NamedDecl *, QuoteTokenRole> Targets;
};

class ResolutionCollector : public RecursiveASTVisitor<ResolutionCollector> {
public:
  ResolutionCollector(
      const SourceManager &SM,
      llvm::DenseMap<unsigned, llvm::DenseMap<unsigned, ExpansionTargets>>
          &Resolutions)
      : SM(SM), Resolutions(Resolutions) {}

  bool VisitDeclRefExpr(DeclRefExpr *E) {
    add(E->getLocation(), E->getDecl(), QuoteTokenRole::Reference);
    addQualifier(E->getQualifierLoc());
    return true;
  }

  bool VisitMemberExpr(MemberExpr *E) {
    add(E->getMemberLoc(), E->getMemberDecl(), QuoteTokenRole::Reference);
    addQualifier(E->getQualifierLoc());
    return true;
  }

  bool VisitCallExpr(CallExpr *E) {
    if (const FunctionDecl *Callee = E->getDirectCallee())
      // The begin location of a qualified callee can be its namespace or type
      // qualifier (e.g. the `util` in `util::worker()`). The callee's end
      // location identifies the name itself, which is the spelling token that
      // should receive the resolution.
      add(E->getCallee()->getEndLoc(), Callee, QuoteTokenRole::Reference);
    return true;
  }

  bool VisitCXXConstructExpr(CXXConstructExpr *E) {
    if (const CXXConstructorDecl *Ctor = E->getConstructor()) {
      const NamedDecl *D = Ctor->getParent();
      if (const auto *CTSD = dyn_cast<ClassTemplateSpecializationDecl>(D))
        D = CTSD->getSpecializedTemplate();
      add(E->getBeginLoc(), D, QuoteTokenRole::Reference);
    }
    return true;
  }

  bool VisitVarDecl(VarDecl *D) {
    add(D->getLocation(), D, QuoteTokenRole::Declaration);
    return true;
  }

  bool VisitFieldDecl(FieldDecl *D) {
    add(D->getLocation(), D, QuoteTokenRole::Declaration);
    return true;
  }

  bool VisitFunctionDecl(FunctionDecl *D) {
    add(D->getLocation(), D, QuoteTokenRole::Declaration);
    return true;
  }

  bool VisitTypeLoc(TypeLoc TL) {
    if (auto RTL = TL.getAs<RecordTypeLoc>()) {
      const NamedDecl *D = RTL.getDecl();
      if (const auto *CTSD = dyn_cast<ClassTemplateSpecializationDecl>(D))
        D = CTSD->getSpecializedTemplate();
      add(RTL.getNameLoc(), D, QuoteTokenRole::Reference);
    } else if (auto TTL = TL.getAs<TypedefTypeLoc>())
      add(TTL.getNameLoc(), TTL.getDecl(), QuoteTokenRole::Reference);
    else if (auto TSTL = TL.getAs<TemplateSpecializationTypeLoc>())
      if (const TemplateDecl *TD =
              TSTL.getTypePtr()->getTemplateName().getAsTemplateDecl())
        add(TSTL.getTemplateNameLoc(), TD, QuoteTokenRole::Reference);
    return true;
  }

private:
  void addQualifier(NestedNameSpecifierLoc Qualifier) {
    while (Qualifier) {
      if (auto Namespace = Qualifier.getAsNamespaceAndPrefix()) {
        add(Qualifier.getLocalBeginLoc(), Namespace.Namespace,
            QuoteTokenRole::Reference);
        Qualifier = Namespace.Prefix;
        continue;
      }
      if (auto TL = Qualifier.getAsTypeLoc()) {
        if (auto RTL = TL.getAs<RecordTypeLoc>())
          add(RTL.getNameLoc(), RTL.getDecl(), QuoteTokenRole::Reference);
        else if (auto TSTL = TL.getAs<TemplateSpecializationTypeLoc>())
          if (const TemplateDecl *TD =
                  TSTL.getTypePtr()->getTemplateName().getAsTemplateDecl())
            add(TSTL.getTemplateNameLoc(), TD, QuoteTokenRole::Reference);
      }
      break;
    }
  }

  void add(SourceLocation Loc, const NamedDecl *D, QuoteTokenRole Role) {
    if (!D || Loc.isInvalid())
      return;
    SourceLocation Spelling = SM.getSpellingLoc(Loc);
    if (Spelling.isInvalid() || SM.getFileID(Spelling) != SM.getMainFileID())
      return;
    unsigned Offset = SM.getFileOffset(Spelling);
    if (!QuoteOffsets.count(Offset))
      return;
    SourceLocation Expansion = SM.getExpansionLoc(Loc);
    if (Expansion.isInvalid())
      return;
    Resolutions[Offset][Expansion.getRawEncoding()]
        .Targets[llvm::cast<NamedDecl>(D->getCanonicalDecl())] = Role;
  }

public:
  void setQuoteOffsets(llvm::DenseSet<unsigned> Offsets) {
    QuoteOffsets = std::move(Offsets);
  }

private:
  const SourceManager &SM;
  llvm::DenseMap<unsigned, llvm::DenseMap<unsigned, ExpansionTargets>>
      &Resolutions;
  llvm::DenseSet<unsigned> QuoteOffsets;
};

bool sameTargets(const ExpansionTargets &L, const ExpansionTargets &R) {
  if (L.Targets.size() != R.Targets.size())
    return false;
  for (const auto &Entry : L.Targets) {
    auto It = R.Targets.find(Entry.first);
    if (It == R.Targets.end() || It->second != Entry.second)
      return false;
  }
  return true;
}

} // namespace

bool QuoteTokenTable::contains(SourceLocation Loc,
                               const SourceManager &SM) const {
  return findToken(Loc, SM) != nullptr;
}

const QuoteTokenTable::TokenInfo *
QuoteTokenTable::findToken(SourceLocation Loc, const SourceManager &SM) const {
  Loc = SM.getSpellingLoc(Loc);
  if (Loc.isInvalid() || SM.getFileID(Loc) != SM.getMainFileID())
    return nullptr;
  unsigned Offset = SM.getFileOffset(Loc);
  auto It = Tokens.find(Offset);
  if (It != Tokens.end())
    return &It->second;
  for (const auto &Entry : Tokens)
    if (Offset > Entry.first && Offset < Entry.first + Entry.second.Length)
      return &Entry.second;
  return nullptr;
}

const QuoteTokenTable::Resolution *
QuoteTokenTable::lookup(SourceLocation Loc, const SourceManager &SM) const {
  const TokenInfo *Info = findToken(Loc, SM);
  return Info ? &Info->Result : nullptr;
}

SourceLocation QuoteTokenTable::tokenLocation(SourceLocation Loc,
                                              const SourceManager &SM) const {
  const TokenInfo *Info = findToken(Loc, SM);
  return Info ? Info->Loc : SourceLocation();
}

llvm::SmallVector<QuoteTokenTable::Reference, 4>
QuoteTokenTable::references(const NamedDecl *D) const {
  llvm::SmallVector<Reference, 4> Result;
  if (!D)
    return Result;
  const NamedDecl *Canonical = llvm::cast<NamedDecl>(D->getCanonicalDecl());
  for (const auto &Entry : Tokens) {
    const Resolution &R = Entry.second.Result;
    if (R.Ambiguous || !R.Expansions)
      continue;
    for (const Target &T : R.Targets)
      if (T.Decl == Canonical)
        Result.push_back({Entry.second.Loc, T.Role});
  }
  return Result;
}

llvm::SmallVector<QuoteTokenTable::Token, 16> QuoteTokenTable::tokens() const {
  llvm::SmallVector<Token, 16> Result;
  Result.reserve(Tokens.size());
  for (const auto &Entry : Tokens)
    Result.push_back({Entry.second.Loc, Entry.second.Length,
                      Entry.second.IsExpressionMacro, Entry.second.Result});
  return Result;
}

QuoteTokenTable QuoteTokenTable::build(ASTContext &Ctx,
                                       llvm::ArrayRef<Decl *> TopLevelDecls) {
  QuoteTokenTable Table;
  const SourceManager &SM = Ctx.getSourceManager();
  QuoteCollector QuotePass(SM, Table);
  for (Decl *D : TopLevelDecls)
    QuotePass.TraverseDecl(D);

  llvm::DenseSet<unsigned> QuoteOffsets;
  for (const auto &Entry : Table.Tokens)
    QuoteOffsets.insert(Entry.first);

  llvm::DenseMap<unsigned, llvm::DenseMap<unsigned, ExpansionTargets>>
      Resolutions;
  ResolutionCollector ResolvePass(SM, Resolutions);
  ResolvePass.setQuoteOffsets(std::move(QuoteOffsets));
  // RecursiveASTVisitor does not visit template instantiations by default, so
  // dependent uses are resolved from the pattern only.
  for (Decl *D : TopLevelDecls)
    ResolvePass.TraverseDecl(D);

  for (auto &Entry : Resolutions) {
    auto TokenIt = Table.Tokens.find(Entry.first);
    if (TokenIt == Table.Tokens.end() || Entry.second.empty())
      continue;
    Resolution &R = TokenIt->second.Result;
    R.Expansions = Entry.second.size();
    const ExpansionTargets *First = nullptr;
    for (const auto &Expansion : Entry.second) {
      if (!First)
        First = &Expansion.second;
      else if (!sameTargets(*First, Expansion.second))
        R.Ambiguous = true;
      for (const auto &ExpansionTarget : Expansion.second.Targets) {
        auto Existing = llvm::find_if(R.Targets, [&](const Target &T) {
          return T.Decl == ExpansionTarget.first &&
                 T.Role == ExpansionTarget.second;
        });
        if (Existing == R.Targets.end())
          R.Targets.push_back({ExpansionTarget.first, ExpansionTarget.second});
      }
    }
  }
  return Table;
}

} // namespace clang::clangd
