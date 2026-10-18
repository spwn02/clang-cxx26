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

  // identifier ('.' identifier)* from \param K; the end is the index after it.
  auto Chain = [&](size_t K, std::string &Name, unsigned &Begin,
                   unsigned &End) -> size_t {
    Name = Toks[K].Text;
    Begin = Toks[K].Begin;
    End = Toks[K].End;
    ++K;
    while (K + 1 < Toks.size() && Toks[K].Kind == tok::period &&
           isIdentifierText(Toks[K + 1].Text)) {
      Name += "." + Toks[K + 1].Text;
      End = Toks[K + 1].End;
      K += 2;
    }
    return K;
  };
  auto IsName = [&](size_t K) {
    return K < Toks.size() && isIdentifierText(Toks[K].Text) &&
           Toks[K].Text != "module" && Toks[K].Text != "import" &&
           Toks[K].Text != "private";
  };

  for (size_t I = 0; I < Toks.size(); ++I) {
    size_t J = I;
    if (!Toks[J].BeginsLine)
      continue;
    if (Toks[J].Text == "export" && J + 1 < Toks.size())
      ++J;
    const llvm::StringRef Keyword = Toks[J].Text;
    if ((Keyword != "module" && Keyword != "import") || J + 1 >= Toks.size())
      continue;
    ModuleNameSpelling Spelling;
    Spelling.IsModuleDeclaration = Keyword == "module";
    // The tokens after the keyword: [identifier ('.' identifier)*] [':'
    // identifier ('.' identifier)*]. The name ends where a partition, an
    // attribute or the semicolon begins.
    size_t K = J + 1;
    if (IsName(K))
      K = Chain(K, Spelling.Name, Spelling.Begin, Spelling.End);
    if (K + 1 < Toks.size() && Toks[K].Kind == tok::colon && IsName(K + 1) &&
        // `module :P;` is not a declaration: a partition follows a name.
        (!Spelling.Name.empty() || Keyword == "import")) {
      Spelling.PartitionColon = Toks[K].Begin;
      K = Chain(K + 1, Spelling.Partition, Spelling.PartitionBegin,
                Spelling.PartitionEnd);
    }
    if (Spelling.Name.empty() && Spelling.Partition.empty())
      continue;
    Result.push_back(std::move(Spelling));
    I = K - 1;
  }
  return Result;
}

std::optional<std::string> moduleOfUnit(llvm::StringRef Code) {
  for (const ModuleNameSpelling &S : moduleNameSpellings(Code))
    if (S.IsModuleDeclaration)
      return S.Name;
  return std::nullopt;
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
    unsigned Begin = S.Name.empty() ? S.PartitionColon : S.Begin;
    unsigned End = S.PartitionEnd ? S.PartitionEnd : S.End;
    if (Offset >= Begin && Offset <= End)
      return std::move(S);
  }
  return std::nullopt;
}

namespace {
// The identifiers of \param Name (separated by '.') are identifiers that are not
// `module` or `import` and not reserved ([module.unit], [lex.name]).
llvm::Error validateIdentifiers(llvm::StringRef Name, llvm::StringRef What,
                                llvm::SmallVectorImpl<llvm::StringRef> &Parts) {
  if (Name.empty())
    return error("the name of a {0} cannot be empty", What);
  Name.split(Parts, '.');
  for (llvm::StringRef Part : Parts) {
    if (!isIdentifierText(Part))
      return error("'{0}' is not a {1} name: '{2}' is not an identifier", Name,
                   What, Part);
    if (Part == "module" || Part == "import")
      return error("'{0}' is not a {1} name: '{2}' cannot be part of it", Name,
                   What, Part);
    // A reserved identifier ([lex.name]): a double underscore, or an underscore
    // and an uppercase letter.
    if (Part.contains("__") ||
        (Part.size() > 1 && Part[0] == '_' && isUppercase(Part[1])))
      return error("'{0}' is reserved: '{1}' is a reserved identifier", Name,
                   Part);
  }
  return llvm::Error::success();
}
} // namespace

llvm::Error validateModuleName(llvm::StringRef Name) {
  llvm::SmallVector<llvm::StringRef, 4> Parts;
  if (llvm::Error Err = validateIdentifiers(Name, "module", Parts))
    return Err;
  llvm::StringRef First = Parts.front();
  if (First.starts_with("std") &&
      llvm::all_of(First.drop_front(3), [](char C) { return isDigit(C); }))
    return error("'{0}' is reserved: module names that begin with 'std' "
                 "followed by digits are reserved",
                 Name);
  return llvm::Error::success();
}

llvm::Error validatePartitionName(llvm::StringRef Name) {
  llvm::SmallVector<llvm::StringRef, 4> Parts;
  return validateIdentifiers(Name, "partition", Parts);
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

namespace {

struct SpelledFile {
  std::string Path;
  std::string Code;
  std::vector<ModuleNameSpelling> Spellings;
  /// The module that the file is a unit of, if it is one.
  std::optional<std::string> Module;
};

// The files of \param Files that spell one of the \param Needles (the last
// parts of the names that matter), read once with their declarations.
std::vector<SpelledFile> spelledFiles(llvm::ArrayRef<std::string> Files,
                                      const ThreadsafeFS &FS,
                                      llvm::ArrayRef<llvm::StringRef> Needles) {
  std::vector<SpelledFile> Result;
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
    SpelledFile Spelled;
    Spelled.Path = File;
    Spelled.Code = (*Buffer)->getBuffer().str();
    if (!llvm::any_of(Needles, [&](llvm::StringRef Needle) {
          return llvm::StringRef(Spelled.Code).contains(Needle);
        }))
      continue;
    Spelled.Spellings = moduleNameSpellings(Spelled.Code);
    for (const ModuleNameSpelling &S : Spelled.Spellings)
      if (S.IsModuleDeclaration) {
        Spelled.Module = S.Name;
        break;
      }
    Result.push_back(std::move(Spelled));
  }
  return Result;
}

llvm::Expected<FileEdits> editsOf(llvm::ArrayRef<SpelledFile> Files,
                                  std::vector<std::vector<Range>> &Ranges,
                                  llvm::StringRef NewName) {
  FileEdits Result;
  for (size_t I = 0; I < Files.size(); ++I) {
    if (Ranges[I].empty())
      continue;
    std::vector<SymbolRange> Occurrences;
    for (const Range &R : Ranges[I])
      Occurrences.emplace_back(R);
    auto Edit = buildRenameEdit(Files[I].Path, Files[I].Code,
                                std::move(Occurrences), NewName);
    if (!Edit)
      return error("failed to rename in file {0}: {1}", Files[I].Path,
                   Edit.takeError());
    Result.insert({Files[I].Path, std::move(*Edit)});
  }
  return Result;
}

llvm::StringRef lastPart(llvm::StringRef Name) {
  return Name.rsplit('.').second;
}

} // namespace

llvm::Expected<FileEdits>
renameModuleInFiles(llvm::StringRef Name, llvm::StringRef NewName,
                    llvm::ArrayRef<std::string> Files, const ThreadsafeFS &FS) {
  std::vector<SpelledFile> Spelled =
      spelledFiles(Files, FS, {lastPart(Name), lastPart(NewName)});
  std::vector<std::vector<Range>> Ranges(Spelled.size());
  for (size_t I = 0; I < Spelled.size(); ++I)
    for (const ModuleNameSpelling &S : Spelled[I].Spellings) {
      if (S.IsModuleDeclaration && S.Name == NewName)
        return error("'{0}' already names a module (declared in {1})", NewName,
                     Spelled[I].Path);
      if (S.Name == Name)
        Ranges[I].push_back(Range{offsetToPosition(Spelled[I].Code, S.Begin),
                                  offsetToPosition(Spelled[I].Code, S.End)});
    }
  return editsOf(Spelled, Ranges, NewName);
}

llvm::Expected<FileEdits>
renamePartitionInFiles(llvm::StringRef Module, llvm::StringRef Partition,
                       llvm::StringRef NewPartition,
                       llvm::ArrayRef<std::string> Files, const ThreadsafeFS &FS) {
  std::vector<SpelledFile> Spelled = spelledFiles(
      Files, FS, {lastPart(Partition), lastPart(NewPartition)});
  // The modules other than \param Module that have a partition of this name:
  // an `import :P;` of a header can be for any of them.
  std::set<std::string> OtherOwners;
  for (const SpelledFile &F : Spelled)
    for (const ModuleNameSpelling &S : F.Spellings)
      if (S.IsModuleDeclaration && S.Partition == Partition && S.Name != Module)
        OtherOwners.insert(S.Name);
  std::vector<std::vector<Range>> Ranges(Spelled.size());
  for (size_t I = 0; I < Spelled.size(); ++I) {
    const SpelledFile &F = Spelled[I];
    for (const ModuleNameSpelling &S : F.Spellings) {
      if (S.Partition.empty())
        continue;
      if (S.IsModuleDeclaration && S.Name == Module &&
          S.Partition == NewPartition)
        return error("the module '{0}' already has a partition '{1}' "
                     "(declared in {2})",
                     Module, NewPartition, F.Path);
      if (S.Partition != Partition)
        continue;
      // The partitions of a module are imported by its own units only
      // ([module.unit]); the module of a header is that of the unit that
      // includes it.
      bool Ours = S.IsModuleDeclaration ? S.Name == Module
                  : F.Module            ? *F.Module == Module
                                        : true;
      if (!Ours)
        continue;
      if (!S.IsModuleDeclaration && !F.Module && !OtherOwners.empty())
        return error("'import :{0};' in {1} can be for the module '{2}' or "
                     "for '{3}': it is not renamed, nothing is renamed",
                     Partition, F.Path, Module, *OtherOwners.begin());
      Ranges[I].push_back(Range{offsetToPosition(F.Code, S.PartitionBegin),
                                offsetToPosition(F.Code, S.PartitionEnd)});
    }
  }
  return editsOf(Spelled, Ranges, NewPartition);
}

} // namespace clangd
} // namespace clang
