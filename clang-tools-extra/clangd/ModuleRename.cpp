//===--- ModuleRename.cpp - Rename of a module ------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ModuleRename.h"
#include "ProjectModules.h"
#include "refactor/Rename.h"
#include "support/Logger.h"
#include "clang/Basic/CharInfo.h"
#include "clang/Basic/LangOptions.h"
#include "clang/Basic/TokenKinds.h"
#include "clang/Tooling/Syntax/Tokens.h"
#include "llvm/ADT/StringSet.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/Path.h"
#include <set>

namespace clang {
namespace clangd {
namespace {

bool isIdentifierText(llvm::StringRef Text) {
  return !Text.empty() && isAsciiIdentifierStart(Text[0]) &&
         llvm::all_of(Text, [](char C) { return isAsciiIdentifierContinue(C); });
}

struct Tok {
  tok::TokenKind Kind;
  std::string Text;
  unsigned Begin, End;
  bool BeginsLine;
};

std::vector<Tok> lexTokens(llvm::StringRef Code) {
  std::vector<Tok> Toks;
  // FIXME: InMemoryFileAdapter crashes unless the buffer is null terminated!
  std::string NullTerminatedCode = Code.str();
  SourceManagerForFile FileSM("mock_file_name.cpp", NullTerminatedCode);
  const SourceManager &SM = FileSM.get();
  LangOptions LangOpts;
  LangOpts.CPlusPlus = LangOpts.CPlusPlus11 = LangOpts.CPlusPlus14 =
      LangOpts.CPlusPlus17 = LangOpts.CPlusPlus20 = LangOpts.CPlusPlus23 =
          LangOpts.CPlusPlus26 = true;
  unsigned PreviousEnd = 0;
  for (const syntax::Token &T : syntax::tokenize(SM.getMainFileID(), SM, LangOpts)) {
    if (T.kind() == tok::eof)
      break;
    unsigned Begin = SM.getFileOffset(T.location());
    unsigned End = Begin + T.length();
    // The token begins its line when a new line (or the start of the file) is
    // all that is before it; comments are white space.
    bool BeginsLine = Toks.empty() ||
                      Code.slice(PreviousEnd, Begin).contains('\n');
    Toks.push_back({T.kind(), T.text(SM).str(), Begin, End, BeginsLine});
    PreviousEnd = End;
  }
  return Toks;
}

} // namespace

std::vector<ModuleNameSpelling> moduleNameSpellings(llvm::StringRef Code) {
  std::vector<ModuleNameSpelling> Result;
  std::vector<Tok> Toks = lexTokens(Code);

  for (size_t I = 0; I < Toks.size(); ++I) {
    size_t J = I;
    if (!Toks[J].BeginsLine)
      continue;
    if (Toks[J].Text == "export" && J + 1 < Toks.size())
      ++J;
    const llvm::StringRef Keyword = Toks[J].Text;
    if ((Keyword != "module" && Keyword != "import") || J + 1 >= Toks.size())
      continue;
    // The tokens after the keyword: identifier ('.' identifier)*.
    size_t K = J + 1;
    if (!isIdentifierText(Toks[K].Text) || Toks[K].Text == "module" ||
        Toks[K].Text == "import")
      continue;
    ModuleNameSpelling Spelling;
    Spelling.IsModuleDeclaration = Keyword == "module";
    Spelling.Begin = Toks[K].Begin;
    Spelling.End = Toks[K].End;
    Spelling.Name = Toks[K].Text;
    ++K;
    while (K + 1 < Toks.size() && Toks[K].Kind == tok::period &&
           isIdentifierText(Toks[K + 1].Text)) {
      Spelling.Name += "." + Toks[K + 1].Text;
      Spelling.End = Toks[K + 1].End;
      K += 2;
    }
    // The name ends where a partition, an attribute or the semicolon begins.
    if (K < Toks.size() && Toks[K].Kind == tok::colon && K + 1 < Toks.size() &&
        isIdentifierText(Toks[K + 1].Text)) {
      Spelling.PartitionBegin = Toks[K].Begin;
      size_t P = K + 1;
      Spelling.PartitionEnd = Toks[P].End;
      ++P;
      while (P + 1 < Toks.size() && Toks[P].Kind == tok::period &&
             isIdentifierText(Toks[P + 1].Text)) {
        Spelling.PartitionEnd = Toks[P + 1].End;
        P += 2;
      }
    }
    Result.push_back(std::move(Spelling));
    I = K ? K - 1 : I;
  }
  return Result;
}

std::vector<std::string> moduleDeclarationSpellings(llvm::StringRef Code) {
  std::vector<std::string> Result;
  std::vector<Tok> Toks = lexTokens(Code);
  for (size_t I = 0; I < Toks.size(); ++I) {
    size_t J = I;
    if (!Toks[J].BeginsLine)
      continue;
    if (Toks[J].Text == "export" && J + 1 < Toks.size())
      ++J;
    const llvm::StringRef Keyword = Toks[J].Text;
    if ((Keyword != "module" && Keyword != "import") || J + 1 >= Toks.size())
      continue;
    // identifier ('.' identifier)* [':' identifier ('.' identifier)*], or
    // ':' identifier ('.' identifier)* for the partition of an import.
    std::string Spelling = Keyword.str() + " ";
    size_t K = J + 1;
    auto Chain = [&]() {
      Spelling += Toks[K].Text;
      ++K;
      while (K + 1 < Toks.size() && Toks[K].Kind == tok::period &&
             isIdentifierText(Toks[K + 1].Text)) {
        Spelling += "." + Toks[K + 1].Text;
        K += 2;
      }
    };
    if (isIdentifierText(Toks[K].Text) && Toks[K].Text != "module" &&
        Toks[K].Text != "import")
      Chain();
    if (K + 1 < Toks.size() && Toks[K].Kind == tok::colon &&
        isIdentifierText(Toks[K + 1].Text)) {
      Spelling += ":";
      ++K;
      Chain();
    }
    // A declaration is complete with its semicolon: until it is typed, the
    // scanner does not see the import.
    bool Terminated = false;
    for (size_t T = K; T < Toks.size() && (T == K || !Toks[T].BeginsLine); ++T)
      if (Toks[T].Kind == tok::semi) {
        Terminated = true;
        break;
      }
    if (Terminated)
      Spelling += " ;";
    if (Spelling.size() > Keyword.size() + 1)
      Result.push_back(std::move(Spelling));
    I = K - 1;
  }
  return Result;
}

std::optional<ModuleNameSpelling> moduleNameSpellingAt(llvm::StringRef Code,
                                                       unsigned Offset) {
  for (ModuleNameSpelling &S : moduleNameSpellings(Code)) {
    unsigned End = S.PartitionEnd ? S.PartitionEnd : S.End;
    if (Offset >= S.Begin && Offset <= End)
      return std::move(S);
  }
  return std::nullopt;
}

llvm::Error validateModuleName(llvm::StringRef Name) {
  if (Name.empty())
    return error("the name of a module cannot be empty");
  llvm::SmallVector<llvm::StringRef, 4> Parts;
  Name.split(Parts, '.');
  for (llvm::StringRef Part : Parts) {
    if (!isIdentifierText(Part))
      return error("'{0}' is not a module name: '{1}' is not an identifier",
                   Name, Part);
    if (Part == "module" || Part == "import")
      return error("'{0}' is not a module name: '{1}' cannot be part of it",
                   Name, Part);
    // A reserved identifier ([lex.name]): a double underscore, or an underscore
    // and an uppercase letter.
    if (Part.contains("__") ||
        (Part.size() > 1 && Part[0] == '_' && isUppercase(Part[1])))
      return error("'{0}' is reserved: '{1}' is a reserved identifier", Name,
                   Part);
  }
  llvm::StringRef First = Parts.front();
  if (First.starts_with("std") &&
      llvm::all_of(First.drop_front(3), [](char C) { return isDigit(C); }))
    return error("'{0}' is reserved: module names that begin with 'std' "
                 "followed by digits are reserved",
                 Name);
  return llvm::Error::success();
}

llvm::Expected<std::vector<std::string>>
findFilesThatCanImport(llvm::ArrayRef<std::string> Roots,
                       const ThreadsafeFS &FS, unsigned MaxDirectories) {
  // The search is bounded so that a workspace root that is a home directory
  // does not hang the editor; when the bound is reached the rename is refused.
  static const llvm::StringSet<> Extensions = {
      ".cpp", ".cc",  ".cxx", ".c++", ".cp",  ".C",   ".cppm", ".ixx", ".cxxm",
      ".c++m", ".mpp", ".h",   ".hpp", ".hh",  ".hxx", ".h++",  ".H",   ".hp",
      ".inc", ".inl", ".ipp", ".tpp", ".tcc", ".ii",  ".ccm"};
  auto View = FS.view(std::nullopt);
  std::set<std::string> Result;
  std::set<std::string> VisitedRoots;
  unsigned Directories = 0;
  bool CutOff = false;
  auto Walk = [&](auto &&Self, llvm::StringRef Dir) -> void {
    if (CutOff)
      return;
    if (++Directories > MaxDirectories) {
      CutOff = true;
      return;
    }
    std::error_code EC;
    for (llvm::vfs::directory_iterator It = View->dir_begin(Dir, EC), End;
         !EC && It != End; It.increment(EC)) {
      llvm::StringRef Path = It->path();
      if (It->type() == llvm::sys::fs::file_type::directory_file) {
        if (!isIgnoredProjectDirectory(llvm::sys::path::filename(Path)))
          Self(Self, Path);
      } else if (It->type() == llvm::sys::fs::file_type::regular_file &&
                 Extensions.contains(llvm::sys::path::extension(Path))) {
        Result.insert(Path.str());
      }
    }
  };
  for (const std::string &Root : Roots)
    if (VisitedRoots.insert(Root).second)
      Walk(Walk, Root);
  if (CutOff)
    return error("the project has more than {0} directories under the "
                 "workspace: the files that import the module cannot all be "
                 "found, nothing is renamed",
                 MaxDirectories);
  return std::vector<std::string>(Result.begin(), Result.end());
}

llvm::Expected<FileEdits>
renameModuleInFiles(llvm::StringRef Name, llvm::StringRef NewName,
                    llvm::ArrayRef<std::string> Files, const ThreadsafeFS &FS) {
  FileEdits Result;
  auto View = FS.view(std::nullopt);
  std::set<std::string> Seen;
  for (const std::string &File : Files) {
    // One file may be given by two spellings (a symbolic link, "a/../b"): it
    // is edited once, under the spelling that came first.
    llvm::SmallString<256> Real;
    if (View->getRealPath(File, Real))
      Real = File;
    if (!Seen.insert(std::string(Real)).second)
      continue;
    auto Buffer = View->getBufferForFile(File);
    if (!Buffer)
      continue; // A file that is gone, or cannot be read, names no module.
    // The text is copied: the file system view does not outlive this call.
    std::string Code = (*Buffer)->getBuffer().str();
    if (!llvm::StringRef(Code).contains(Name.rsplit('.').second))
      continue;
    std::vector<SymbolRange> Occurrences;
    for (const ModuleNameSpelling &S : moduleNameSpellings(Code))
      if (S.Name == Name)
        Occurrences.emplace_back(
            Range{offsetToPosition(Code, S.Begin), offsetToPosition(Code, S.End)});
    if (Occurrences.empty())
      continue;
    llvm::StringRef New = NewName;
    auto Edit = buildRenameEdit(File, Code, std::move(Occurrences), New);
    if (!Edit)
      return error("failed to rename in file {0}: {1}", File, Edit.takeError());
    Result.insert({File, std::move(*Edit)});
  }
  return Result;
}

} // namespace clangd
} // namespace clang
