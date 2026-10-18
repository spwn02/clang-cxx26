//===--- ModuleImports.cpp - Import declarations as the preprocessor reads -===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ModuleImports.h"
#include "Compiler.h"
#include "Diagnostics.h"
#include "SourceCode.h"
#include "support/Logger.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Lex/Lexer.h"
#include "clang/Lex/MacroInfo.h"
#include "clang/Lex/PPCallbacks.h"
#include "clang/Lex/Preprocessor.h"
#include "llvm/ADT/DenseSet.h"
#include <set>

namespace clang {
namespace clangd {
namespace {

class ImportCollector : public PPCallbacks {
public:
  ImportCollector(Preprocessor &PP, std::vector<ResolvedImport> &Out)
      : PP(PP), SM(PP.getSourceManager()), Out(Out) {}

  void moduleImport(SourceLocation ImportLoc, ModuleIdPath Path,
                    const Module *) override {
    if (!ImportLoc.isFileID() || Path.empty() || !Path[0].getIdentifierInfo())
      return;
    FileID FID = SM.getFileID(ImportLoc);
    OptionalFileEntryRef FE = SM.getFileEntryRefForID(FID);
    if (!FE)
      return;
    ResolvedImport R;
    R.Path = pathOf(*FE);
    R.Offset = SM.getFileOffset(ImportLoc);
    R.Name = Path[0].getIdentifierInfo()->getName().str();

    // The tokens of the declaration, read again as spelled: the preprocessor
    // flattens the name, and keeps the place of its first identifier only.
    llvm::StringRef Buffer = SM.getBufferData(FID);
    Lexer L(SM.getLocForStartOfFile(FID), PP.getLangOpts(), Buffer.begin(),
            Buffer.begin() + R.Offset, Buffer.end());
    Token Tok;
    L.LexFromRawLexer(Tok); // `import`
    std::vector<Piece> Pieces;
    std::set<const IdentifierInfo *> Hidden;
    bool Failed = false;
    for (L.LexFromRawLexer(Tok); Tok.isNot(tok::eof) && Tok.isNot(tok::semi);
         L.LexFromRawLexer(Tok)) {
      if (Tok.isOneOf(tok::l_square, tok::less, tok::string_literal,
                      tok::header_name))
        break; // An attribute follows the name, or this is a header unit.
      if (!expand(Tok, Pieces, Hidden, R.Unresolved)) {
        Failed = true;
        break;
      }
    }
    if (!Failed)
      assemble(R, Pieces);
    Out.push_back(std::move(R));
  }

private:
  struct Piece {
    tok::TokenKind Kind;
    std::string Text;
    SpelledIdentifier Where;
  };

  std::string pathOf(FileEntryRef FE) const {
    if (auto Real = getCanonicalPath(FE, SM.getFileManager()))
      return *Real;
    return FE.getName().str();
  }

  SpelledIdentifier whereIs(SourceLocation Loc, unsigned Length) const {
    SpelledIdentifier W;
    Loc = SM.getFileLoc(Loc);
    FileID FID = SM.getFileID(Loc);
    if (OptionalFileEntryRef FE = SM.getFileEntryRefForID(FID)) {
      W.Path = pathOf(*FE);
      W.Begin = SM.getFileOffset(Loc);
      W.End = W.Begin + Length;
    }
    return W;
  }

  // Appends the tokens that \param T stands for after macro replacement, with
  // the places where they are spelled. False when it cannot be known.
  bool expand(const Token &T, std::vector<Piece> &Pieces,
              std::set<const IdentifierInfo *> &Hidden, std::string &Why) {
    if (T.is(tok::raw_identifier) || T.is(tok::identifier)) {
      Token Copy = T;
      IdentifierInfo *II = Copy.is(tok::raw_identifier)
                               ? PP.LookUpIdentifierInfo(Copy)
                               : Copy.getIdentifierInfo();
      if (II && !Hidden.count(II))
        if (const MacroInfo *MI = PP.getMacroInfo(II)) {
          if (MI->isFunctionLike()) {
            Why = ("'" + II->getName() + "' is a function-like macro").str();
            return false;
          }
          Hidden.insert(II);
          bool Ok = true;
          for (const Token &Body : MI->tokens())
            if (!(Ok = expand(Body, Pieces, Hidden, Why)))
              break;
          Hidden.erase(II);
          return Ok;
        }
      Pieces.push_back({tok::identifier, II ? II->getName().str() : "",
                        whereIs(T.getLocation(), II ? II->getName().size() : 0)});
      return true;
    }
    Pieces.push_back({T.getKind(), "", {}});
    return true;
  }

  // identifier ('.' identifier)* [':' identifier ('.' identifier)*]
  void assemble(ResolvedImport &R, const std::vector<Piece> &Pieces) {
    std::string Flat;
    bool InPartition = false;
    bool ExpectIdentifier = true;
    for (const Piece &P : Pieces) {
      if (P.Kind == tok::identifier && ExpectIdentifier) {
        Flat += P.Text;
        (InPartition ? R.PartitionIdentifiers : R.NameIdentifiers)
            .push_back(P.Where);
        ExpectIdentifier = false;
      } else if (P.Kind == tok::period && !ExpectIdentifier) {
        Flat += ".";
        ExpectIdentifier = true;
      } else if (P.Kind == tok::colon && !InPartition) {
        Flat += ":";
        InPartition = true;
        ExpectIdentifier = true;
      } else {
        R.NameIdentifiers.clear();
        R.PartitionIdentifiers.clear();
        R.Unresolved = "the name is not made of identifiers";
        return;
      }
    }
    // `import :P;` is the partition of the module of its own unit, which the
    // preprocessor names.
    bool Matches = Flat == R.Name ||
                   (llvm::StringRef(Flat).starts_with(":") &&
                    llvm::StringRef(R.Name).ends_with(Flat));
    if (!Matches) {
      R.NameIdentifiers.clear();
      R.PartitionIdentifiers.clear();
      R.Unresolved = "the spelling '" + Flat + "' is not the name '" + R.Name + "'";
    }
  }

  Preprocessor &PP;
  const SourceManager &SM;
  std::vector<ResolvedImport> &Out;
};

} // namespace

llvm::Expected<std::vector<ResolvedImport>>
resolveImports(PathRef File, const tooling::CompileCommand &Cmd,
               const ThreadsafeFS &FS) {
  auto View = FS.view(Cmd.Directory);
  auto Buffer = View->getBufferForFile(File);
  if (!Buffer)
    return error("cannot read {0}: {1}", File, Buffer.getError().message());
  ParseInputs Inputs;
  Inputs.CompileCommand = Cmd;
  Inputs.TFS = &FS;
  Inputs.Contents = (*Buffer)->getBuffer().str();
  IgnoreDiagnostics IgnoreDiags;
  auto CI = buildCompilerInvocation(Inputs, IgnoreDiags);
  if (!CI)
    return error("cannot read the compile command of {0}", File);
  auto Clang = prepareCompilerInstance(
      std::move(CI), /*Preamble=*/nullptr,
      llvm::MemoryBuffer::getMemBufferCopy(Inputs.Contents, File), View,
      IgnoreDiags);
  if (!Clang)
    return error("cannot prepare the preprocessor for {0}", File);
  std::vector<ResolvedImport> Imports;
  PreprocessOnlyAction Action;
  if (!Action.BeginSourceFile(*Clang, Clang->getFrontendOpts().Inputs[0]))
    return error("cannot preprocess {0}", File);
  Clang->getPreprocessor().addPPCallbacks(
      std::make_unique<ImportCollector>(Clang->getPreprocessor(), Imports));
  llvm::Error Err = Action.Execute();
  Action.EndSourceFile();
  if (Err)
    return std::move(Err);
  return Imports;
}

} // namespace clangd
} // namespace clang
