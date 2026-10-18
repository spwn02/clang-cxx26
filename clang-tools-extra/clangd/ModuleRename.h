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

#include "SourceCode.h"
#include "support/Path.h"
#include "support/ThreadsafeFS.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Error.h"
#include <optional>
#include <string>
#include <vector>

namespace clang {
namespace clangd {

/// The primary module name of a module declaration or an import declaration,
/// as spelled in a file.
struct ModuleNameSpelling {
  /// The whole dotted name ("A.B"), without the partition.
  std::string Name;
  /// Where the dotted name is spelled (offsets in the code).
  unsigned Begin = 0, End = 0;
  /// The declaration is a module-declaration (not an import-declaration).
  bool IsModuleDeclaration = false;
  /// The partition of the declaration ("module A:P;"), spelled after the name.
  /// Zero when there is none.
  unsigned PartitionBegin = 0, PartitionEnd = 0;
};

/// The module names that the declarations of \param Code spell, in order. A
/// declaration begins its line ([cpp.module], [cpp.import]): `[export] module
/// N [:P] ... ;` and `[export] import N [:P] ... ;`. `import :P;`, `module;`,
/// `module :private;`, header units, text of comments and literals and
/// declarations that do not begin their line are not names.
std::vector<ModuleNameSpelling> moduleNameSpellings(llvm::StringRef Code);

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

/// The edits that rename the module \param Name to \param NewName in \param
/// Files (read through \param FS, which has the unsaved text of the open
/// files). With more than \param LimitFiles files to edit the rename is
/// refused.
llvm::Expected<FileEdits>
renameModuleInFiles(llvm::StringRef Name, llvm::StringRef NewName,
                    llvm::ArrayRef<std::string> Files, const ThreadsafeFS &FS,
                    size_t LimitFiles);

} // namespace clangd
} // namespace clang

#endif
