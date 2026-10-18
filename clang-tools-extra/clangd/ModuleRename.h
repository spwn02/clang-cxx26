//===--- ModuleRename.h - Rename of a module ---------------------*- C++-*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// A module is named by the module-name of a module-declaration
// ([module.unit]) and of an import-declaration ([module.import]); both are
// found by their tokens, in the files that the project has, and the files that
// are open in the editor.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANGD_MODULERENAME_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANGD_MODULERENAME_H

#include "ModuleImports.h"
#include "SourceCode.h"
#include "support/Path.h"
#include "support/ThreadsafeFS.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Error.h"
#include <functional>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace clang {
namespace clangd {

/// The names of a module declaration or an import declaration, as spelled in a
/// file: the primary module name and the partition.
struct ModuleNameSpelling {
  /// The whole dotted name ("A.B"), without the partition. Empty in
  /// `import :P;`, which names a partition of the module of its own unit.
  std::string Name;
  /// Where the dotted name is spelled (offsets in the code); zero when there
  /// is no name.
  unsigned Begin = 0, End = 0;
  /// The declaration is a module-declaration (not an import-declaration).
  bool IsModuleDeclaration = false;
  /// The partition of the declaration ("module A:P;", "import :P;"): the whole
  /// dotted name after the colon ("P", "a.b"). Empty when there is none.
  std::string Partition;
  /// Where the colon is, and where the partition name is spelled. Zero when
  /// there is no partition.
  unsigned PartitionColon = 0, PartitionBegin = 0, PartitionEnd = 0;
  /// Where the `module` or `import` of the declaration is.
  unsigned Keyword = 0;

  /// Whether \param Offset is in the partition (the colon belongs to the
  /// name in front of it, what follows the colon to the partition).
  bool isInPartition(unsigned Offset) const {
    return PartitionEnd && (Name.empty() ? Offset >= PartitionColon
                                         : Offset > PartitionColon);
  }
};

/// The names that the declarations of \param Code spell, in order. A
/// declaration begins its line ([cpp.module], [cpp.import]): `[export] module
/// N [:P] ... ;` and `[export] import N ... ;` or `[export] import :P ... ;`.
/// `module;`, `module :private;`, header units, text of comments and literals
/// and declarations that do not begin their line are not names.
std::vector<ModuleNameSpelling> moduleNameSpellings(llvm::StringRef Code);

/// The name of the module that the unit \param Code is a unit of (the name of
/// its module-declaration), if it is one.
std::optional<std::string> moduleOfUnit(llvm::StringRef Code);

/// The module-declaration and the import-declarations of \param Code, as
/// spelled ("module A.B:P", "import C", "import :P"), in order. What a unit
/// needs from other modules is known by them.
std::vector<std::string> moduleDeclarationSpellings(llvm::StringRef Code);

/// The spelling at \param Offset (inside the name, its partition, or right
/// after them), if any.
std::optional<ModuleNameSpelling> moduleNameSpellingAt(llvm::StringRef Code,
                                                       unsigned Offset);

/// Whether \param Name can be the name of a module that a module-declaration
/// specifies: identifiers separated by '.', none of them `module` or `import`,
/// the first not `std` followed by digits, none reserved ([module.unit]).
/// Returns the reason when not.
llvm::Error validateModuleName(llvm::StringRef Name);

/// Whether \param Name can be the name of a partition: the same, except that
/// the rule about `std` is for the names of modules.
llvm::Error validatePartitionName(llvm::StringRef Name);

/// The files under \param Roots (read through \param FS) that can spell the
/// name of a module: sources, module units and headers, found by their
/// extension; not the directories of build products. A header may import a
/// module, a unit that only imports one is not listed by the compilation
/// database. Fails when the search is cut off, instead of returning a part of
/// the files: a rename that misses a file breaks the project.
llvm::Expected<std::vector<std::string>>
findFilesThatCanImport(llvm::ArrayRef<std::string> Roots, const ThreadsafeFS &FS,
                       unsigned MaxDirectories = 50000);

/// What the rename needs to know when an import declaration names a module
/// through a macro ([cpp.import] processes its tokens as normal text).
struct RenameEnvironment {
  /// The names of the macros that the compile commands define (-DNAME).
  std::set<std::string> CommandLineMacros;
  /// The import declarations of a translation unit of the project, as its
  /// preprocessor reads them. Fails for a file that is not one.
  std::function<llvm::Expected<std::vector<ResolvedImport>>(llvm::StringRef)>
      ResolveImports;
};

/// The edits that rename the module \param Name to \param NewName in \param
/// Files (read through \param FS, which has the unsaved text of the open
/// files). The number of files is not limited: the rename of a module changes
/// every file that imports it, and a rename that stops half way leaves the
/// project broken.
///
/// The rename is refused when a module declaration in \param Files already
/// names \param NewName.
llvm::Expected<FileEdits>
renameModuleInFiles(llvm::StringRef Name, llvm::StringRef NewName,
                    llvm::ArrayRef<std::string> Files, const ThreadsafeFS &FS,
                    const RenameEnvironment &Env = RenameEnvironment());

/// The edits that rename the partition \param Partition of the module \param
/// Module to \param NewPartition: the module declarations `Module:Partition`
/// and the imports `import :Partition;` of the units of \param Module. A file
/// without a module declaration (a header) that spells `import :Partition;`
/// is edited when it can belong to \param Module only: no other module has a
/// partition of that name. The rename is refused when \param Module already
/// has a partition \param NewPartition.
llvm::Expected<FileEdits>
renamePartitionInFiles(llvm::StringRef Module, llvm::StringRef Partition,
                       llvm::StringRef NewPartition,
                       llvm::ArrayRef<std::string> Files, const ThreadsafeFS &FS,
                       const RenameEnvironment &Env = RenameEnvironment());

} // namespace clangd
} // namespace clang

#endif
