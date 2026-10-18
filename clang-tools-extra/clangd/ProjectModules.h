//===------------------ ProjectModules.h -------------------------*- C++-*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANGD_PROJECTMODULES_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANGD_PROJECTMODULES_H

#include "support/Function.h"
#include "support/Path.h"
#include "support/ThreadsafeFS.h"
#include "clang/Tooling/CompilationDatabase.h"

#include <memory>
#include <optional>

namespace clang {
namespace clangd {

/// An interface to query the modules information in the project.
/// Users should get instances of `ProjectModules` from
/// `GlobalCompilationDatabase::getProjectModules(PathRef)`.
///
/// Currently, the modules information includes:
/// - Given a source file, what are the required modules.
/// - Given a module name and a required source file, what is
///   the corresponding source file.
///
/// Note that there can be multiple source files declaring the same module
/// in a valid project. Although the language specification requires that
/// every module unit's name must be unique in valid program, there can be
/// multiple program in a project. And it is technically valid if these program
/// doesn't interfere with each other.
///
/// A module name should be in the format:
/// `<primary-module-name>[:partition-name]`. So module names covers partitions.
class ProjectModules {
public:
  using CommandMangler =
      llvm::unique_function<void(tooling::CompileCommand &, PathRef) const>;

  virtual std::vector<std::string> getRequiredModules(PathRef File) = 0;
  virtual std::string getModuleNameForSource(PathRef File) = 0;
  virtual std::string getSourceForModuleName(llvm::StringRef ModuleName,
                                             PathRef RequiredSrcFile) = 0;

  /// The compile command to build the module unit \param File with, if the
  /// project does not have it in its compilation database. This is the case for
  /// the module units of the standard library (std, std.compat), which live in
  /// the toolchain and are built with the flags of the file that imports them.
  virtual std::optional<tooling::CompileCommand>
  getCompileCommandForSource(PathRef File) {
    return std::nullopt;
  }

  /// The names of the modules that an import declaration in \param
  /// ImportingFile can name: the modules found so far, the module units that
  /// the compilation database does not list (searched for, at most once a
  /// second) and the modules of the standard library of the toolchain that the
  /// file is built with. With \param ScanProject the units of the database are
  /// scanned too (once; the result is kept), otherwise the caller knows them
  /// already (a build system that names every module file).
  virtual std::vector<std::string> getKnownModuleNames(PathRef ImportingFile,
                                                       bool ScanProject) {
    return {};
  }

  /// The source files of the project: the files of the compilation database
  /// and the module units that it does not list.
  virtual std::vector<std::string> getSourceFiles() { return {}; }

  /// The outermost directories that hold the files of the compilation
  /// database. Files that the database does not list (a unit that only imports
  /// a module, a header) are in them, or in the workspace.
  virtual std::vector<std::string> getSourceDirectories() { return {}; }

  virtual void setCommandMangler(CommandMangler Mangler) {}

  /// The file system that the sources are read through. A module unit that is
  /// open in the editor, and not saved, is known by its unsaved text.
  virtual void setFileSystem(const ThreadsafeFS &TFS) {}

  virtual ~ProjectModules() = default;
};

/// Whether a directory of this name holds build products or third-party code
/// that is fetched, rather than the sources of the project.
bool isIgnoredProjectDirectory(llvm::StringRef Name);

} // namespace clangd
} // namespace clang

#endif
