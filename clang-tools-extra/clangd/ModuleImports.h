//===--- ModuleImports.h - Import declarations as the preprocessor reads ---===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// The tokens after `import` are processed as in normal text ([cpp.import]):
// `import M;` with `#define M Foo` imports the module Foo. The name of a
// module in a module declaration is never macro replaced ([cpp.module]).
// The identifiers of the name of an import declaration are found here by the
// preprocessor of the translation unit, with the places where they are spelled.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANGD_MODULEIMPORTS_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANGD_MODULEIMPORTS_H

#include "support/Path.h"
#include "support/ThreadsafeFS.h"
#include "clang/Tooling/CompilationDatabase.h"
#include "llvm/Support/Error.h"
#include <string>
#include <vector>

namespace clang {
namespace clangd {

/// An identifier of the name of a module, where it is spelled.
struct SpelledIdentifier {
  /// The file; empty when the identifier is spelled on the command line (or
  /// in another buffer that is not a file) and cannot be edited.
  std::string Path;
  unsigned Begin = 0, End = 0;
};

/// An import declaration as the preprocessor reads it.
struct ResolvedImport {
  /// The file of the declaration and the offset of its `import`.
  std::string Path;
  unsigned Offset = 0;
  /// The name that the declaration imports: "A.B", "A.B:P" or, for `import
  /// :P;` in a unit of A, "A:P".
  std::string Name;
  /// The identifiers of the module name (not of the partition), and of the
  /// partition, in order. Empty when the declaration could not be resolved
  /// (a function-like macro): \c Unresolved says why.
  std::vector<SpelledIdentifier> NameIdentifiers, PartitionIdentifiers;
  std::string Unresolved;
};

/// The import declarations of the translation unit \param File (and of the
/// files it includes) that spell a name, as the preprocessor of its compile
/// command \param Cmd reads them.
llvm::Expected<std::vector<ResolvedImport>>
resolveImports(PathRef File, const tooling::CompileCommand &Cmd,
               const ThreadsafeFS &FS);

} // namespace clangd
} // namespace clang

#endif
