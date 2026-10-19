//===------------ ScanningProjectModules.h -----------------------*- C++-*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANGD_SCANNINGPROJECTMODULES_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANGD_SCANNINGPROJECTMODULES_H

#include "ProjectModules.h"
#include "clang/Tooling/CompilationDatabase.h"
#include <functional>
#include <optional>

namespace clang {
namespace clangd {

/// What the scans of one compilation database found. Share one between the
/// ProjectModules of the same database to avoid scanning files again that did
/// not change.
class ModuleScanCache;
class ModulesActivity;
/// \param Activity is told about the scans that take time (may be null).
std::shared_ptr<ModuleScanCache>
createModuleScanCache(std::shared_ptr<ModulesActivity> Activity = nullptr);

/// The flags that select the libc++ that clangd itself belongs to (the headers
/// installed next to its binary), or none if there is none. The command of a
/// file without a compilation database gets them: its `#include <vector>` and
/// the `import std;` that is built from the same libc++ then agree.
std::vector<std::string> ownLibcxxFlags();

/// A project without a compilation database to read the sources from (or with
/// one that lists no files, such as compile_flags.txt): the tree below Root is
/// searched for the sources, and the files that the database has no command
/// for get FallbackCommand.
struct ProjectTree {
  std::string Root;
  std::function<tooling::CompileCommand(PathRef File)> FallbackCommand;
};

/// Providing modules information for the project by scanning every file. Files
/// that the database does not list (a new module unit) are found in the
/// directories of the files it lists. \param CDB may be null if \param Tree
/// is given.
std::unique_ptr<ProjectModules> scanningProjectModules(
    std::shared_ptr<const clang::tooling::CompilationDatabase> CDB,
    const ThreadsafeFS &TFS, std::shared_ptr<ModuleScanCache> Cache = nullptr,
    std::optional<ProjectTree> Tree = std::nullopt);

} // namespace clangd
} // namespace clang

#endif
