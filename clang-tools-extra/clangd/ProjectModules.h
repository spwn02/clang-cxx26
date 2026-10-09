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

  virtual void setCommandMangler(CommandMangler Mangler) {}

  virtual ~ProjectModules() = default;
};

} // namespace clangd
} // namespace clang

#endif
