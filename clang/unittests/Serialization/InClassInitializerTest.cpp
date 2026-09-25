//===- unittests/Serialization/InClassInitializerTest.cpp -----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Basic/FileManager.h"
#include "clang/Driver/CreateInvocationFromArgs.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/CompilerInvocation.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Frontend/Utils.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"

#include "gtest/gtest.h"

using namespace llvm;
using namespace clang;

namespace {

// The default member initializer of a field read from a module or PCH is
// deserialized lazily. It must be deserialized once and then cached on the
// FieldDecl: every call used to build a new expression tree, so the parent map
// (built by one traversal) did not contain the nodes seen by a later traversal
// ("Tried to match orphan node" in assertion-enabled clang-tidy builds).
TEST(InClassInitializerTest, LazyInitializerIsCached) {
  SmallString<256> TestDir;
  ASSERT_FALSE(sys::fs::createUniqueDirectory("in-class-init-test", TestDir));

  SmallString<256> ModulePath(TestDir);
  sys::path::append(ModulePath, "M.cppm");
  {
    std::error_code EC;
    raw_fd_ostream OS(ModulePath, EC);
    ASSERT_FALSE(EC);
    OS << "export module M;\n"
          "export struct S { long c = 1; unsigned d{2}; };\n";
  }

  CreateInvocationOptions CIOpts;
  CIOpts.VFS = vfs::createPhysicalFileSystem();
  DiagnosticOptions DiagOpts;
  IntrusiveRefCntPtr<DiagnosticsEngine> Diags =
      CompilerInstance::createDiagnostics(*CIOpts.VFS, DiagOpts);
  CIOpts.Diags = Diags;

  const char *Args[] = {"clang++",       "-std=c++20",    "--precompile",
                        "-working-directory", TestDir.c_str(), "M.cppm"};
  std::shared_ptr<CompilerInvocation> Invocation =
      createInvocation(Args, CIOpts);
  ASSERT_TRUE(Invocation);
  Invocation->getFrontendOpts().DisableFree = false;

  CompilerInstance Instance(std::move(Invocation));
  Instance.setDiagnostics(Diags);

  std::string BMIPath = (TestDir + "/M.pcm").str();
  Instance.getFrontendOpts().OutputFile = BMIPath;

  GenerateReducedModuleInterfaceAction Action;
  ASSERT_TRUE(Instance.ExecuteAction(Action));
  ASSERT_FALSE(Diags->hasErrorOccurred());

  std::string DepArg = "-fmodule-file=M=" + BMIPath;
  std::unique_ptr<ASTUnit> AST = tooling::buildASTFromCodeWithArgs(
      "import M;\n", /*Args=*/{"-std=c++20", DepArg.c_str()});
  ASSERT_TRUE(AST);

  using namespace clang::ast_matchers;
  ASTContext &Ctx = AST->getASTContext();
  for (const char *Name : {"c", "d"}) {
    const auto *FD = selectFirst<FieldDecl>(
        "field", match(fieldDecl(hasName(Name)).bind("field"), Ctx));
    ASSERT_TRUE(FD) << Name;
    ASSERT_TRUE(FD->hasInClassInitializer()) << Name;
    const Expr *First = FD->getInClassInitializer();
    ASSERT_TRUE(First) << Name;
    EXPECT_EQ(First, FD->getInClassInitializer()) << Name;
  }

  sys::fs::remove_directories(TestDir);
}

} // anonymous namespace
