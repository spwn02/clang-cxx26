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
#include <map>
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
    Spelling.Keyword = Toks[J].Begin;
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
  std::string Real;
  std::string Code;
  std::vector<ModuleNameSpelling> Spellings;
  /// The module that the file is a unit of, if it is one.
  std::optional<std::string> Module;
};

struct Replace {
  unsigned Begin, End;
  std::string Text;
  bool operator<(const Replace &O) const {
    return std::tie(Begin, End, Text) < std::tie(O.Begin, O.End, O.Text);
  }
};

// The names of the macros that \param Code defines.
void collectMacroNames(llvm::StringRef Code, std::set<std::string> &Names) {
  for (llvm::StringRef Line : llvm::split(Code, '\n')) {
    Line = Line.ltrim(" \t");
    if (!Line.consume_front("#"))
      continue;
    Line = Line.ltrim(" \t");
    if (!Line.consume_front("define") && !Line.consume_front("undef"))
      continue;
    if (Line.empty() || (Line[0] != ' ' && Line[0] != '\t'))
      continue;
    Line = Line.ltrim(" \t");
    size_t N = 0;
    while (N < Line.size() && isAsciiIdentifierContinue(Line[N]))
      ++N;
    if (N)
      Names.insert(Line.take_front(N).str());
  }
}

// The files of \param Files that spell one of the \param Needles (the last
// parts of the names that matter) or an import declaration (a macro may stand
// for the name), read once with their declarations. \param Macros receives the
// names of the macros that the files define.
std::vector<SpelledFile> spelledFiles(llvm::ArrayRef<std::string> Files,
                                      const ThreadsafeFS &FS,
                                      llvm::ArrayRef<llvm::StringRef> Needles,
                                      std::set<std::string> &Macros) {
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
    Spelled.Real = std::string(Real);
    Spelled.Code = (*Buffer)->getBuffer().str();
    llvm::StringRef Code = Spelled.Code;
    if (Code.contains("define") || Code.contains("undef"))
      collectMacroNames(Code, Macros);
    if (!Code.contains("import") &&
        !llvm::any_of(Needles, [&](llvm::StringRef N) { return Code.contains(N); }))
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

// Whether a name of the import declaration \param S can stand for something
// else than it spells: one of its identifiers is a macro.
bool namedByMacro(const ModuleNameSpelling &S,
                  const std::set<std::string> &Macros) {
  if (S.IsModuleDeclaration)
    return false; // No macro replacement in a module declaration.
  for (llvm::StringRef Name : {llvm::StringRef(S.Name), llvm::StringRef(S.Partition)}) {
    llvm::SmallVector<llvm::StringRef, 4> Parts;
    Name.split(Parts, '.', -1, /*KeepEmpty=*/false);
    for (llvm::StringRef Part : Parts)
      if (Macros.count(Part.str()))
        return true;
  }
  return false;
}

// The edits that make the identifiers \param Ids (the places where a name is
// spelled, perhaps in a macro) say \param NewName.
llvm::Error identifierEdits(llvm::ArrayRef<SpelledIdentifier> Ids,
                            llvm::StringRef NewName, llvm::StringRef Importer,
                            std::vector<std::pair<std::string, Replace>> &Out,
                            const std::function<llvm::StringRef(llvm::StringRef)> &TextOf) {
  llvm::SmallVector<llvm::StringRef, 4> Parts;
  NewName.split(Parts, '.');
  for (const SpelledIdentifier &Id : Ids)
    if (Id.Path.empty())
      return error("the import in {0} gets its name from a macro of the "
                   "command line: it cannot be renamed, nothing is renamed",
                   Importer);
  if (Ids.size() == Parts.size()) {
    for (size_t I = 0; I < Ids.size(); ++I)
      Out.push_back({Ids[I].Path, {Ids[I].Begin, Ids[I].End, Parts[I].str()}});
    return llvm::Error::success();
  }
  // Another number of identifiers: only a name that is spelled in one piece
  // (only dots and white space between its identifiers) can be replaced.
  bool Contiguous = true;
  for (size_t I = 1; I < Ids.size(); ++I) {
    llvm::StringRef Text = TextOf(Ids[0].Path);
    if (Ids[I].Path != Ids[0].Path || Ids[I].Begin < Ids[I - 1].End ||
        Ids[I].End > Text.size() ||
        !llvm::all_of(Text.slice(Ids[I - 1].End, Ids[I].Begin),
                      [](char C) { return C == '.' || isWhitespace(C); }))
      Contiguous = false;
  }
  if (!Contiguous)
    return error("the import in {0} spells the name in pieces (through "
                 "macros) that the new name does not fit, nothing is renamed",
                 Importer);
  Out.push_back({Ids[0].Path, {Ids[0].Begin, Ids.back().End, NewName.str()}});
  return llvm::Error::success();
}

// The edits that rename \param Target: \c matches selects what an import
// declaration or a module declaration spells that is renamed. Partition edits
// when \param Partition is not empty.
struct RenameTarget {
  std::string Module;
  std::string Partition; // Empty: the module itself.
  std::string NewName;
  // The name that \c ResolvedImport::Name has when it is the target.
  std::string resolvedName() const {
    return Partition.empty() ? Module : Module + ":" + Partition;
  }
};

llvm::Expected<FileEdits> renameInFiles(const RenameTarget &T,
                                        llvm::ArrayRef<std::string> Files,
                                        const ThreadsafeFS &FS,
                                        const RenameEnvironment &Env) {
  const bool IsPartition = !T.Partition.empty();
  std::set<std::string> Macros = Env.CommandLineMacros;
  // Any identifier of the names can be spelled alone, by a macro.
  std::vector<std::string> NeedleStorage;
  for (llvm::StringRef Name : {llvm::StringRef(IsPartition ? T.Partition : T.Module),
                               llvm::StringRef(T.NewName)}) {
    llvm::SmallVector<llvm::StringRef, 4> Parts;
    Name.split(Parts, '.');
    for (llvm::StringRef Part : Parts)
      NeedleStorage.push_back(Part.str());
  }
  std::vector<llvm::StringRef> Needles(NeedleStorage.begin(), NeedleStorage.end());
  std::vector<SpelledFile> Spelled = spelledFiles(Files, FS, Needles, Macros);

  // The modules other than the target that have a partition of this name: an
  // `import :P;` of a header can be for any of them.
  std::set<std::string> OtherOwners;
  if (IsPartition)
    for (const SpelledFile &F : Spelled)
      for (const ModuleNameSpelling &S : F.Spellings)
        if (S.IsModuleDeclaration && S.Partition == T.Partition &&
            S.Name != T.Module)
          OtherOwners.insert(S.Name);

  // The import declarations that macros may change are read by the
  // preprocessor of the translation unit that has them (or includes them).
  std::map<std::pair<std::string, unsigned>, ResolvedImport> Resolved;
  std::set<std::string> Preprocessed;
  std::map<std::string, std::string> NotAUnit;
  for (const SpelledFile &F : Spelled) {
    if (!llvm::any_of(F.Spellings, [&](const ModuleNameSpelling &S) {
          return namedByMacro(S, Macros);
        }))
      continue;
    if (!Env.ResolveImports) {
      NotAUnit[F.Real] = "the preprocessor was not available";
      continue;
    }
    auto Imports = Env.ResolveImports(F.Path);
    if (!Imports) {
      NotAUnit[F.Real] = llvm::toString(Imports.takeError());
      continue;
    }
    Preprocessed.insert(F.Real);
    for (ResolvedImport &R : *Imports)
      Resolved.insert({{R.Path, R.Offset}, std::move(R)});
  }

  std::map<std::string, std::vector<Replace>> Replaces;
  std::vector<std::pair<std::string, Replace>> Pending;
  std::map<std::string, std::string> Texts; // By path, for identifierEdits.
  for (const SpelledFile &F : Spelled)
    Texts[F.Real] = F.Code;
  auto TextOf = [&](llvm::StringRef Path) -> llvm::StringRef {
    auto It = Texts.find(Path.str());
    return It == Texts.end() ? llvm::StringRef() : llvm::StringRef(It->second);
  };

  for (const SpelledFile &F : Spelled) {
    for (const ModuleNameSpelling &S : F.Spellings) {
      if (!IsPartition) {
        if (S.IsModuleDeclaration && S.Name == T.NewName)
          return error("'{0}' already names a module (declared in {1})",
                       T.NewName, F.Path);
      } else if (S.IsModuleDeclaration && S.Name == T.Module &&
                 S.Partition == T.NewName) {
        return error("the module '{0}' already has a partition '{1}' "
                     "(declared in {2})",
                     T.Module, T.NewName, F.Path);
      }

      if (namedByMacro(S, Macros)) {
        // What the declaration imports is known by the preprocessor.
        auto It = Resolved.find({F.Real, S.Keyword});
        if (It == Resolved.end()) {
          if (Preprocessed.count(F.Real))
            continue; // Not read: a branch that the configuration skips.
          auto Why = NotAUnit.find(F.Real);
          return error("the import in {0} may name the module through a "
                       "macro, and the file is not read by the preprocessor "
                       "of a source of the project ({1}): nothing is renamed",
                       F.Path,
                       Why == NotAUnit.end() ? std::string("unknown")
                                             : Why->second);
        }
        const ResolvedImport &R = It->second;
        if (!R.Unresolved.empty())
          return error("the import in {0} names a module through a macro that "
                       "cannot be followed ({1}): nothing is renamed",
                       F.Path, R.Unresolved);
        const bool NameMatches =
            IsPartition ? R.Name == T.resolvedName()
                        : llvm::StringRef(R.Name).split(':').first == T.Module &&
                              !R.NameIdentifiers.empty();
        if (!NameMatches)
          continue;
        if (auto Err = identifierEdits(
                IsPartition ? R.PartitionIdentifiers : R.NameIdentifiers,
                T.NewName, F.Path, Pending, TextOf))
          return std::move(Err);
        continue;
      }

      if (S.Partition.empty() && IsPartition)
        continue;
      if (!IsPartition) {
        if (S.Name == T.Module && !S.Name.empty())
          Pending.push_back({F.Real, {S.Begin, S.End, T.NewName}});
        continue;
      }
      if (S.Partition != T.Partition)
        continue;
      // The partitions of a module are imported by its own units only
      // ([module.unit]); the module of a header is that of the unit that
      // includes it.
      bool Ours = S.IsModuleDeclaration ? S.Name == T.Module
                  : F.Module            ? *F.Module == T.Module
                                        : true;
      if (!Ours)
        continue;
      if (!S.IsModuleDeclaration && !F.Module && !OtherOwners.empty())
        return error("'import :{0};' in {1} can be for the module '{2}' or "
                     "for '{3}': it is not renamed, nothing is renamed",
                     T.Partition, F.Path, T.Module, *OtherOwners.begin());
      Pending.push_back({F.Real, {S.PartitionBegin, S.PartitionEnd, T.NewName}});
    }
  }

  // The edits, by the files as they were given.
  std::map<std::string, std::set<Replace>> ByFile;
  for (auto &[Path, R] : Pending)
    ByFile[Path].insert(std::move(R));
  FileEdits Result;
  for (const SpelledFile &F : Spelled) {
    auto It = ByFile.find(F.Real);
    if (It == ByFile.end())
      continue;
    tooling::Replacements Reps;
    for (const Replace &R : It->second)
      if (auto Err = Reps.add(tooling::Replacement(F.Path, R.Begin,
                                                   R.End - R.Begin, R.Text)))
        return error("failed to rename in file {0}: {1}", F.Path,
                     std::move(Err));
    Result.insert({F.Path, Edit(F.Code, std::move(Reps))});
    ByFile.erase(It);
  }
  // A name spelled in a file that was not searched (a macro in a header the
  // project does not list): the rename cannot reach it.
  if (!ByFile.empty())
    return error("the name is spelled in {0}, which is not a file of the "
                 "project: nothing is renamed",
                 ByFile.begin()->first);
  return Result;
}

} // namespace

llvm::Expected<FileEdits>
renameModuleInFiles(llvm::StringRef Name, llvm::StringRef NewName,
                    llvm::ArrayRef<std::string> Files, const ThreadsafeFS &FS,
                    const RenameEnvironment &Env) {
  return renameInFiles({Name.str(), "", NewName.str()}, Files, FS, Env);
}

llvm::Expected<FileEdits>
renamePartitionInFiles(llvm::StringRef Module, llvm::StringRef Partition,
                       llvm::StringRef NewPartition,
                       llvm::ArrayRef<std::string> Files, const ThreadsafeFS &FS,
                       const RenameEnvironment &Env) {
  return renameInFiles({Module.str(), Partition.str(), NewPartition.str()},
                       Files, FS, Env);
}

} // namespace clangd
} // namespace clang
