//===--------------- PrerequisiteModulesTests.cpp -------------------*- C++
//-*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

/// FIXME: Skip testing on windows temporarily due to the different escaping
/// code mode.
#ifndef _WIN32

#include "Annotations.h"
#include "CodeComplete.h"
#include "Compiler.h"
#include "ClangdServer.h"
#include "ModulesBuilder.h"
#include "ScanningProjectModules.h"
#include "SyncAPI.h"
#include "TestTU.h"
#include "support/ThreadsafeFS.h"
#include "support/Path.h"
#include "llvm/ADT/ScopeExit.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace clang::clangd {
namespace {

class GlobalScanningCounterProjectModules : public ProjectModules {
public:
  GlobalScanningCounterProjectModules(
      std::unique_ptr<ProjectModules> Underlying, std::atomic<unsigned> &Count)
      : Underlying(std::move(Underlying)), Count(Count) {}

  std::vector<std::string> getRequiredModules(PathRef File) override {
    return Underlying->getRequiredModules(File);
  }

  std::string getModuleNameForSource(PathRef File) override {
    return Underlying->getModuleNameForSource(File);
  }

  void setCommandMangler(CommandMangler Mangler) override {
    Underlying->setCommandMangler(std::move(Mangler));
  }

  std::optional<tooling::CompileCommand>
  getCompileCommandForSource(PathRef File) override {
    return Underlying->getCompileCommandForSource(File);
  }

  std::string getSourceForModuleName(llvm::StringRef ModuleName,
                                     PathRef RequiredSrcFile) override {
    Count++;
    return Underlying->getSourceForModuleName(ModuleName, RequiredSrcFile);
  }

private:
  std::unique_ptr<ProjectModules> Underlying;
  std::atomic<unsigned> &Count;
};

class MockDirectoryCompilationDatabase : public MockCompilationDatabase {
public:
  MockDirectoryCompilationDatabase(StringRef TestDir, const ThreadsafeFS &TFS)
      : MockCompilationDatabase(TestDir),
        MockedCDBPtr(std::make_shared<MockClangCompilationDatabase>(*this)),
        TFS(TFS), GlobalScanningCount(0) {
    this->ExtraClangFlags.push_back("-std=c++20");
    this->ExtraClangFlags.push_back("-c");
  }

  void addFile(llvm::StringRef Path, llvm::StringRef Contents);

  // Write a file to the working testing directory that the compilation
  // database does not list (a file the build system has not seen yet).
  void addUnlistedFile(llvm::StringRef Path, llvm::StringRef Contents);

  // The program name in the compile commands, if not "clang".
  std::string FakeCompiler;

  std::optional<tooling::CompileCommand>
  getCompileCommand(PathRef File) const override {
    auto Cmd = MockCompilationDatabase::getCompileCommand(File);
    if (!Cmd)
      return Cmd;
    // Like a real database, name the file by its path relative to the working
    // directory of the command, which differs from the file name in a
    // subdirectory.
    if (llvm::sys::path::is_absolute(File) && File.starts_with(Cmd->Directory))
      Cmd->Filename = llvm::sys::path::relative_path(
                          File.drop_front(Cmd->Directory.size())).str();
    if (!FakeCompiler.empty())
      Cmd->CommandLine.front() = FakeCompiler;
    return Cmd;
  }

  std::unique_ptr<ProjectModules> getProjectModules(PathRef) const override {
    return std::make_unique<GlobalScanningCounterProjectModules>(
        scanningProjectModules(MockedCDBPtr, TFS, ScanCache),
        GlobalScanningCount);
  }

  unsigned getGlobalScanningCount() const { return GlobalScanningCount; }

private:
  class MockClangCompilationDatabase : public tooling::CompilationDatabase {
  public:
    MockClangCompilationDatabase(MockDirectoryCompilationDatabase &MCDB)
        : MCDB(MCDB) {}

    std::vector<tooling::CompileCommand>
    getCompileCommands(StringRef FilePath) const override {
      std::optional<tooling::CompileCommand> Cmd =
          MCDB.getCompileCommand(FilePath);
      EXPECT_TRUE(Cmd);
      return {*Cmd};
    }

    std::vector<std::string> getAllFiles() const override { return Files; }

    void AddFile(StringRef File) { Files.push_back(File.str()); }

  private:
    MockDirectoryCompilationDatabase &MCDB;
    std::vector<std::string> Files;
  };

  std::shared_ptr<MockClangCompilationDatabase> MockedCDBPtr;
  const ThreadsafeFS &TFS;
  // Shared by the requests, like the one of the directory-based database.
  std::shared_ptr<ModuleScanCache> ScanCache = createModuleScanCache();

  mutable std::atomic<unsigned> GlobalScanningCount;
};

void MockDirectoryCompilationDatabase::addUnlistedFile(
    llvm::StringRef Path, llvm::StringRef Contents) {
  ASSERT_FALSE(llvm::sys::path::is_absolute(Path));

  SmallString<256> AbsPath(Directory);
  llvm::sys::path::append(AbsPath, Path);

  ASSERT_FALSE(
      llvm::sys::fs::create_directories(llvm::sys::path::parent_path(AbsPath)));

  std::error_code EC;
  llvm::raw_fd_ostream OS(AbsPath, EC);
  ASSERT_FALSE(EC);
  OS << Contents;
}

// Add files to the working testing directory and the compilation database.
void MockDirectoryCompilationDatabase::addFile(llvm::StringRef Path,
                                               llvm::StringRef Contents) {
  ASSERT_FALSE(llvm::sys::path::is_absolute(Path));

  SmallString<256> AbsPath(Directory);
  llvm::sys::path::append(AbsPath, Path);

  ASSERT_FALSE(
      llvm::sys::fs::create_directories(llvm::sys::path::parent_path(AbsPath)));

  std::error_code EC;
  llvm::raw_fd_ostream OS(AbsPath, EC);
  ASSERT_FALSE(EC);
  OS << Contents;

  MockedCDBPtr->AddFile(Path);
}

class PrerequisiteModulesTests : public ::testing::Test {
protected:
  void SetUp() override {
    ASSERT_FALSE(llvm::sys::fs::createUniqueDirectory("modules-test", TestDir));
  }

  void TearDown() override {
    ASSERT_FALSE(llvm::sys::fs::remove_directories(TestDir));
  }

public:
  // Get the absolute path for file specified by Path under testing working
  // directory.
  std::string getFullPath(llvm::StringRef Path) {
    SmallString<128> Result(TestDir);
    llvm::sys::path::append(Result, Path);
    EXPECT_TRUE(llvm::sys::fs::exists(Result.str()));
    return Result.str().str();
  }

  ParseInputs getInputs(llvm::StringRef FileName,
                        const GlobalCompilationDatabase &CDB) {
    std::string FullPathName = getFullPath(FileName);

    ParseInputs Inputs;
    std::optional<tooling::CompileCommand> Cmd =
        CDB.getCompileCommand(FullPathName);
    EXPECT_TRUE(Cmd);
    Inputs.CompileCommand = std::move(*Cmd);
    Inputs.TFS = &FS;

    if (auto Contents = FS.view(TestDir)->getBufferForFile(FullPathName))
      Inputs.Contents = Contents->get()->getBuffer().str();

    return Inputs;
  }

  SmallString<256> TestDir;
  // FIXME: It will be better to use the MockFS if the scanning process and
  // build module process doesn't depend on reading real IO.
  RealThreadsafeFS FS;

  DiagnosticConsumer DiagConsumer;
};

TEST_F(PrerequisiteModulesTests, NonModularTest) {
  MockDirectoryCompilationDatabase CDB(TestDir, FS);

  CDB.addFile("foo.h", R"cpp(
inline void foo() {}
  )cpp");

  CDB.addFile("NonModular.cpp", R"cpp(
#include "foo.h"
void use() {
  foo();
}
  )cpp");

  ModulesBuilder Builder(CDB);

  // NonModular.cpp is not related to modules. So nothing should be built.
  auto NonModularInfo =
      Builder.buildPrerequisiteModulesFor(getFullPath("NonModular.cpp"), FS);
  EXPECT_TRUE(NonModularInfo);

  HeaderSearchOptions HSOpts;
  NonModularInfo->adjustHeaderSearchOptions(HSOpts);
  EXPECT_TRUE(HSOpts.PrebuiltModuleFiles.empty());

  auto Invocation =
      buildCompilerInvocation(getInputs("NonModular.cpp", CDB), DiagConsumer);
  EXPECT_TRUE(NonModularInfo->canReuse(*Invocation, FS.view(TestDir)));
}

// Writes M.cppm and Use.cpp ("import M;") into the directory of the CDB and
// returns the path of a module file for M that the compiler can use.
class BuiltModuleForM {
public:
  BuiltModuleForM(PrerequisiteModulesTests &T, const ThreadsafeFS &FS,
                  llvm::StringRef Contents = "export module M;\n")
      : CDB(T.TestDir, FS), Builder(CDB) {
    CDB.addFile("M.cppm", Contents);
    CDB.addFile("Use.cpp", "import M;\n");
    Info = Builder.buildPrerequisiteModulesFor(T.getFullPath("Use.cpp"), FS);
    HeaderSearchOptions HS(T.TestDir);
    Info->adjustHeaderSearchOptions(HS);
    Path = HS.PrebuiltModuleFiles["M"];
  }
  // The module file stays on disk while this object lives.
  std::string Path;

private:
  MockDirectoryCompilationDatabase CDB;
  ModulesBuilder Builder;
  std::unique_ptr<PrerequisiteModules> Info;
};

TEST_F(PrerequisiteModulesTests, BuildSystemProvidedModules) {
  BuiltModuleForM Built(*this, FS);
  ASSERT_FALSE(Built.Path.empty());

  // The build system gives a module file that is usable: clangd uses it as it
  // is and does not scan the project.
  MockDirectoryCompilationDatabase CDB(TestDir, FS);
  CDB.ExtraClangFlags.push_back("-fmodule-file=M=" + Built.Path);
  CDB.addFile("M.cppm", "export module M;\n");
  CDB.addFile("Use.cpp", "import M;\n");

  ModulesBuilder Builder(CDB);
  auto Info = Builder.buildPrerequisiteModulesFor(getFullPath("Use.cpp"), FS);
  ASSERT_TRUE(Info);
  EXPECT_EQ(CDB.getGlobalScanningCount(), 0u);
  HeaderSearchOptions HSOpts(TestDir);
  Info->adjustHeaderSearchOptions(HSOpts);
  EXPECT_EQ(HSOpts.PrebuiltModuleFiles["M"], Built.Path);
  auto Invocation =
      buildCompilerInvocation(getInputs("Use.cpp", CDB), DiagConsumer);
  EXPECT_TRUE(Info->canReuse(*Invocation, FS.view(TestDir)));
}

TEST_F(PrerequisiteModulesTests, BuildSystemModulesThatAreMissingAreNotUsed) {
  MockDirectoryCompilationDatabase CDB(TestDir, FS);
  CDB.ExtraClangFlags.push_back("-fmodule-file=M=/nonexistent/M.pcm");
  CDB.addFile("M.cppm", "export module M;\n");
  CDB.addFile("Use.cpp", "import M;\n");

  ModulesBuilder Builder(CDB);
  auto Info = Builder.buildPrerequisiteModulesFor(getFullPath("Use.cpp"), FS);
  ASSERT_TRUE(Info);
  // clangd scanned the project and built M itself.
  EXPECT_GT(CDB.getGlobalScanningCount(), 0u);
  HeaderSearchOptions HSOpts(TestDir);
  Info->adjustHeaderSearchOptions(HSOpts);
  EXPECT_TRUE(StringRef(HSOpts.PrebuiltModuleFiles["M"]).ends_with(".pcm"));
  EXPECT_NE(HSOpts.PrebuiltModuleFiles["M"], "/nonexistent/M.pcm");
}

TEST_F(PrerequisiteModulesTests, BuildSystemModulesInAnUnreadableFormatAreNotUsed) {
  // A module file written by another version of the compiler cannot be read.
  SmallString<256> Bad(TestDir);
  llvm::sys::path::append(Bad, "bad.pcm");
  {
    std::error_code EC;
    llvm::raw_fd_ostream OS(Bad, EC);
    ASSERT_FALSE(EC);
    OS << "this is not a module file";
  }
  MockDirectoryCompilationDatabase CDB(TestDir, FS);
  CDB.ExtraClangFlags.push_back("-fmodule-file=M=" + Bad.str().str());
  CDB.addFile("M.cppm", "export module M;\n");
  CDB.addFile("Use.cpp", "import M;\n");

  ModulesBuilder Builder(CDB);
  auto Info = Builder.buildPrerequisiteModulesFor(getFullPath("Use.cpp"), FS);
  ASSERT_TRUE(Info);
  EXPECT_GT(CDB.getGlobalScanningCount(), 0u);
  HeaderSearchOptions HSOpts(TestDir);
  Info->adjustHeaderSearchOptions(HSOpts);
  EXPECT_NE(HSOpts.PrebuiltModuleFiles["M"], Bad.str().str());
}

TEST_F(PrerequisiteModulesTests, BuildSystemModulesOlderThanTheirSourcesAreNotUsed) {
  BuiltModuleForM Built(*this, FS);
  ASSERT_FALSE(Built.Path.empty());

  // The module unit changes after the build system built its module file.
  MockDirectoryCompilationDatabase CDB(TestDir, FS);
  CDB.ExtraClangFlags.push_back("-fmodule-file=M=" + Built.Path);
  CDB.addFile("M.cppm", "export module M;\nexport int changed();\n");
  CDB.addFile("Use.cpp", "import M;\n");

  ModulesBuilder Builder(CDB);
  auto Info = Builder.buildPrerequisiteModulesFor(getFullPath("Use.cpp"), FS);
  ASSERT_TRUE(Info);
  EXPECT_GT(CDB.getGlobalScanningCount(), 0u);
  HeaderSearchOptions HSOpts(TestDir);
  Info->adjustHeaderSearchOptions(HSOpts);
  EXPECT_NE(HSOpts.PrebuiltModuleFiles["M"], Built.Path);
}

TEST_F(PrerequisiteModulesTests, StandardLibraryModulesFromTheManifest) {
  // A toolchain with the manifest libc++ installs; the project does not have
  // the module unit of std in its compilation database.
  SmallString<256> ToolchainDir;
  ASSERT_FALSE(llvm::sys::fs::createUniqueDirectory("modules-toolchain", ToolchainDir));
  auto Cleanup = llvm::make_scope_exit(
      [&] { llvm::sys::fs::remove_directories(ToolchainDir); });
  auto Write = [&](llvm::StringRef Rel, llvm::StringRef Contents) {
    SmallString<256> Abs(ToolchainDir);
    llvm::sys::path::append(Abs, Rel);
    ASSERT_FALSE(llvm::sys::fs::create_directories(llvm::sys::path::parent_path(Abs)));
    std::error_code EC;
    llvm::raw_fd_ostream OS(Abs, EC);
    ASSERT_FALSE(EC);
    OS << Contents;
  };
  Write("tc/lib/libc++.modules.json", R"json({
  "version": 1, "revision": 1,
  "modules": [
    {"logical-name": "std", "source-path": "../share/std.cppm", "is-std-library": true},
    {"logical-name": "std.compat", "source-path": "../share/std.compat.cppm", "is-std-library": true}
  ]
})json");
  Write("tc/share/std.cppm", "export module std;\nexport namespace std { inline int one() { return 1; } }\n");
  Write("tc/share/std.compat.cppm", "export module std.compat;\nexport import std;\n");

  MockDirectoryCompilationDatabase CDB(TestDir, FS);
  SmallString<256> Compiler(ToolchainDir);
  llvm::sys::path::append(Compiler, "tc", "bin", "clang++");
  CDB.FakeCompiler = std::string(Compiler);
  CDB.addFile("Use.cpp", "import std.compat;\nint f() { return std::one(); }\n");

  ModulesBuilder Builder(CDB);
  auto Info = Builder.buildPrerequisiteModulesFor(getFullPath("Use.cpp"), FS);
  ASSERT_TRUE(Info);
  HeaderSearchOptions HSOpts(TestDir);
  Info->adjustHeaderSearchOptions(HSOpts);
  EXPECT_TRUE(HSOpts.PrebuiltModuleFiles.count("std"));
  EXPECT_TRUE(HSOpts.PrebuiltModuleFiles.count("std.compat"));

  auto Inputs = getInputs("Use.cpp", CDB);
  auto Invocation = buildCompilerInvocation(Inputs, DiagConsumer);
  EXPECT_TRUE(Info->canReuse(*Invocation, FS.view(TestDir)));
}

TEST_F(PrerequisiteModulesTests, ModuleUnitsThatTheDatabaseDoesNotList) {
  MockDirectoryCompilationDatabase CDB(TestDir, FS);

  // The build system has not seen these yet: a module unit next to the listed
  // files and one in a directory below, and an interface unit in a .cpp file.
  CDB.addFile("Use.cpp", "import Fresh;\nimport Fresh.Part;\nimport InCpp;\n");
  CDB.addUnlistedFile("Fresh.cppm", "export module Fresh;\nexport import Fresh.Part;\n");
  CDB.addUnlistedFile("sub/Part.ixx", "export module Fresh.Part;\n");
  CDB.addUnlistedFile("InCpp.cpp", "// comment\nexport module InCpp;\n");
  // Not a module unit, and a build directory: ignored.
  CDB.addUnlistedFile("plain.cpp", "int x;\n");
  CDB.addUnlistedFile("build/Stale.cppm", "export module Fresh;\n");

  ModulesBuilder Builder(CDB);
  auto Info = Builder.buildPrerequisiteModulesFor(getFullPath("Use.cpp"), FS);
  ASSERT_TRUE(Info);
  HeaderSearchOptions HSOpts(TestDir);
  Info->adjustHeaderSearchOptions(HSOpts);
  EXPECT_TRUE(HSOpts.PrebuiltModuleFiles.count("Fresh"));
  EXPECT_TRUE(HSOpts.PrebuiltModuleFiles.count("Fresh.Part"));
  EXPECT_TRUE(HSOpts.PrebuiltModuleFiles.count("InCpp"));
}

TEST_F(PrerequisiteModulesTests, ScansOfUnchangedFilesAreKeptBetweenRequests) {
  MockDirectoryCompilationDatabase CDB(TestDir, FS);
  CDB.addFile("M.cppm", "export module M;\n");
  CDB.addFile("Use.cpp", "import M;\n");

  ModulesBuilder Builder(CDB);
  {
    auto Info = Builder.buildPrerequisiteModulesFor(getFullPath("Use.cpp"), FS);
    HeaderSearchOptions HSOpts(TestDir);
    Info->adjustHeaderSearchOptions(HSOpts);
    EXPECT_TRUE(HSOpts.PrebuiltModuleFiles.count("M"));
  }

  // The module unit now declares another module and the importer follows: the
  // cached scan of M.cppm must not be used for the old name.
  CDB.addFile("M.cppm", "export module M2;\n");
  CDB.addFile("Use.cpp", "import M2;\n");
  {
    auto Info = Builder.buildPrerequisiteModulesFor(getFullPath("Use.cpp"), FS);
    ASSERT_TRUE(Info);
    HeaderSearchOptions HSOpts(TestDir);
    Info->adjustHeaderSearchOptions(HSOpts);
    EXPECT_TRUE(HSOpts.PrebuiltModuleFiles.count("M2"));
    EXPECT_FALSE(HSOpts.PrebuiltModuleFiles.count("M"));
  }

  // A module unit that appears after an earlier request is found.
  CDB.addUnlistedFile("sub/Late.cppm", "export module Late;\n");
  CDB.addFile("Use.cpp", "import M2;\nimport Late;\n");
  {
    auto Info = Builder.buildPrerequisiteModulesFor(getFullPath("Use.cpp"), FS);
    ASSERT_TRUE(Info);
    HeaderSearchOptions HSOpts(TestDir);
    Info->adjustHeaderSearchOptions(HSOpts);
    EXPECT_TRUE(HSOpts.PrebuiltModuleFiles.count("Late"));
  }

  // And one that is deleted is not used any more.
  llvm::sys::fs::remove(getFullPath("sub/Late.cppm"));
  CDB.addFile("Use.cpp", "import Late;\n");
  {
    auto Info = Builder.buildPrerequisiteModulesFor(getFullPath("Use.cpp"), FS);
    ASSERT_TRUE(Info);
    HeaderSearchOptions HSOpts(TestDir);
    Info->adjustHeaderSearchOptions(HSOpts);
    EXPECT_FALSE(HSOpts.PrebuiltModuleFiles.count("Late"));
  }
}

// Records the errors of the last diagnostics of each file.
class ErrorRecorder : public ClangdServer::Callbacks {
public:
  void onDiagnosticsReady(PathRef File, llvm::StringRef Version,
                          llvm::ArrayRef<Diag> Diagnostics) override {
    std::lock_guard<std::mutex> Lock(Mutex);
    std::vector<std::string> &Errors = Last[File];
    Errors.clear();
    for (const Diag &D : Diagnostics)
      if (D.Severity >= DiagnosticsEngine::Error)
        Errors.push_back(D.Message);
  }

  std::vector<std::string> errors(PathRef File) {
    std::lock_guard<std::mutex> Lock(Mutex);
    return Last[File];
  }

private:
  std::mutex Mutex;
  llvm::StringMap<std::vector<std::string>> Last;
};

TEST_F(PrerequisiteModulesTests, EditsOfAModuleUnitReachItsOpenImporters) {
  MockDirectoryCompilationDatabase CDB(TestDir, FS);
  CDB.addFile("M.cppm", "export module M;\nexport int g();\n");
  CDB.addFile("Use.cpp", "import M;\nint f() { return g(); }\n");

  ModulesBuilder Builder(CDB);
  ClangdServer::Options Opts = ClangdServer::optsForTest();
  Opts.ModulesManager = &Builder;
  ErrorRecorder Recorder;
  ClangdServer Server(CDB, FS, Opts, &Recorder);

  std::string UsePath = getFullPath("Use.cpp");
  runAddDocument(Server, UsePath, "import M;\nint f() { return g(); }\n");
  EXPECT_THAT(Recorder.errors(UsePath), ::testing::IsEmpty());

  // The module unit is edited on disk (and saved by the editor): g is gone.
  CDB.addFile("M.cppm", "export module M;\nexport int hh();\n");
  Server.reparseOpenFilesIfNeeded([](llvm::StringRef) { return true; });
  ASSERT_TRUE(Server.blockUntilIdleForTest());
  EXPECT_THAT(Recorder.errors(UsePath), ::testing::Not(::testing::IsEmpty()));

  // A change that the editor reports as a file event (another tool, a branch
  // switch) reaches the importer too.
  CDB.addFile("M.cppm", "export module M;\nexport int g();\nexport int h2();\n");
  DidChangeWatchedFilesParams Event;
  Event.changes.push_back(
      {URIForFile::canonicalize(getFullPath("M.cppm"), TestDir),
       FileChangeType::Changed});
  Server.onFileEvent(Event);
  ASSERT_TRUE(Server.blockUntilIdleForTest());
  EXPECT_THAT(Recorder.errors(UsePath), ::testing::IsEmpty());
}

TEST_F(PrerequisiteModulesTests, ModulesThatExistAreUsedWhenAnotherIsMissing) {
  MockDirectoryCompilationDatabase CDB(TestDir, FS);
  CDB.addFile("M.cppm", "export module M;\n");
  CDB.addFile("Use.cpp", "import M;\nimport Missing;\n");

  ModulesBuilder Builder(CDB);
  auto Info = Builder.buildPrerequisiteModulesFor(getFullPath("Use.cpp"), FS);
  ASSERT_TRUE(Info);
  HeaderSearchOptions HSOpts(TestDir);
  Info->adjustHeaderSearchOptions(HSOpts);
  // M is available, so that the only error is the module that does not exist.
  EXPECT_TRUE(HSOpts.PrebuiltModuleFiles.count("M"));
  EXPECT_FALSE(HSOpts.PrebuiltModuleFiles.count("Missing"));
  // The preamble is built again next time: the missing module may exist then.
  auto Invocation =
      buildCompilerInvocation(getInputs("Use.cpp", CDB), DiagConsumer);
  EXPECT_FALSE(Info->canReuse(*Invocation, FS.view(TestDir)));
}

TEST_F(PrerequisiteModulesTests, ModuleWithoutDepTest) {
  MockDirectoryCompilationDatabase CDB(TestDir, FS);

  CDB.addFile("foo.h", R"cpp(
inline void foo() {}
  )cpp");

  CDB.addFile("M.cppm", R"cpp(
module;
#include "foo.h"
export module M;
  )cpp");

  ModulesBuilder Builder(CDB);

  auto MInfo = Builder.buildPrerequisiteModulesFor(getFullPath("M.cppm"), FS);
  EXPECT_TRUE(MInfo);

  // Nothing should be built since M doesn't dependent on anything.
  HeaderSearchOptions HSOpts;
  MInfo->adjustHeaderSearchOptions(HSOpts);
  EXPECT_TRUE(HSOpts.PrebuiltModuleFiles.empty());

  auto Invocation =
      buildCompilerInvocation(getInputs("M.cppm", CDB), DiagConsumer);
  EXPECT_TRUE(MInfo->canReuse(*Invocation, FS.view(TestDir)));
}

TEST_F(PrerequisiteModulesTests, ModuleWithArgumentPatch) {
  MockDirectoryCompilationDatabase CDB(TestDir, FS);

  CDB.ExtraClangFlags.push_back("-invalid-unknown-flag");

  CDB.addFile("Dep.cppm", R"cpp(
export module Dep;
  )cpp");

  CDB.addFile("M.cppm", R"cpp(
export module M;
import Dep;
  )cpp");

  // An invalid flag will break the module compilation and the
  // getRequiredModules would return an empty array
  auto ProjectModules = CDB.getProjectModules(getFullPath("M.cppm"));
  EXPECT_TRUE(
      ProjectModules->getRequiredModules(getFullPath("M.cppm")).empty());

  // Set the mangler to filter out the invalid flag
  ProjectModules->setCommandMangler([](tooling::CompileCommand &Command,
                                       PathRef) {
    auto const It = llvm::find(Command.CommandLine, "-invalid-unknown-flag");
    Command.CommandLine.erase(It);
  });

  // And now it returns a non-empty list of required modules since the
  // compilation succeeded
  EXPECT_FALSE(
      ProjectModules->getRequiredModules(getFullPath("M.cppm")).empty());
}

TEST_F(PrerequisiteModulesTests, ModuleWithDepTest) {
  MockDirectoryCompilationDatabase CDB(TestDir, FS);

  CDB.addFile("foo.h", R"cpp(
inline void foo() {}
  )cpp");

  CDB.addFile("M.cppm", R"cpp(
module;
#include "foo.h"
export module M;
  )cpp");

  CDB.addFile("N.cppm", R"cpp(
export module N;
import :Part;
import M;
  )cpp");

  CDB.addFile("N-part.cppm", R"cpp(
// Different module name with filename intentionally.
export module N:Part;
  )cpp");

  ModulesBuilder Builder(CDB);

  auto NInfo = Builder.buildPrerequisiteModulesFor(getFullPath("N.cppm"), FS);
  EXPECT_TRUE(NInfo);

  ParseInputs NInput = getInputs("N.cppm", CDB);
  std::unique_ptr<CompilerInvocation> Invocation =
      buildCompilerInvocation(NInput, DiagConsumer);
  // Test that `PrerequisiteModules::canReuse` works basically.
  EXPECT_TRUE(NInfo->canReuse(*Invocation, FS.view(TestDir)));

  {
    // Check that
    // `PrerequisiteModules::adjustHeaderSearchOptions(HeaderSearchOptions&)`
    // can appending HeaderSearchOptions correctly.
    HeaderSearchOptions HSOpts;
    NInfo->adjustHeaderSearchOptions(HSOpts);

    EXPECT_TRUE(HSOpts.PrebuiltModuleFiles.count("M"));
    EXPECT_TRUE(HSOpts.PrebuiltModuleFiles.count("N:Part"));
  }

  {
    // Check that
    // `PrerequisiteModules::adjustHeaderSearchOptions(HeaderSearchOptions&)`
    // can replace HeaderSearchOptions correctly.
    HeaderSearchOptions HSOpts;
    HSOpts.PrebuiltModuleFiles["M"] = "incorrect_path";
    HSOpts.PrebuiltModuleFiles["N:Part"] = "incorrect_path";
    NInfo->adjustHeaderSearchOptions(HSOpts);

    EXPECT_TRUE(StringRef(HSOpts.PrebuiltModuleFiles["M"]).ends_with(".pcm"));
    EXPECT_TRUE(
        StringRef(HSOpts.PrebuiltModuleFiles["N:Part"]).ends_with(".pcm"));
  }
}

TEST_F(PrerequisiteModulesTests, ReusabilityTest) {
  MockDirectoryCompilationDatabase CDB(TestDir, FS);

  CDB.addFile("foo.h", R"cpp(
inline void foo() {}
  )cpp");

  CDB.addFile("M.cppm", R"cpp(
module;
#include "foo.h"
export module M;
  )cpp");

  CDB.addFile("N.cppm", R"cpp(
export module N;
import :Part;
import M;
  )cpp");

  CDB.addFile("N-part.cppm", R"cpp(
// Different module name with filename intentionally.
export module N:Part;
  )cpp");

  ModulesBuilder Builder(CDB);

  auto NInfo = Builder.buildPrerequisiteModulesFor(getFullPath("N.cppm"), FS);
  EXPECT_TRUE(NInfo);
  EXPECT_TRUE(NInfo);

  ParseInputs NInput = getInputs("N.cppm", CDB);
  std::unique_ptr<CompilerInvocation> Invocation =
      buildCompilerInvocation(NInput, DiagConsumer);
  EXPECT_TRUE(NInfo->canReuse(*Invocation, FS.view(TestDir)));

  // Test that we can still reuse the NInfo after we touch a unrelated file.
  {
    CDB.addFile("L.cppm", R"cpp(
module;
#include "foo.h"
export module L;
export int ll = 43;
  )cpp");
    EXPECT_TRUE(NInfo->canReuse(*Invocation, FS.view(TestDir)));

    CDB.addFile("bar.h", R"cpp(
inline void bar() {}
inline void bar(int) {}
  )cpp");
    EXPECT_TRUE(NInfo->canReuse(*Invocation, FS.view(TestDir)));
  }

  // Test that we can't reuse the NInfo after we touch a related file.
  {
    CDB.addFile("M.cppm", R"cpp(
module;
#include "foo.h"
export module M;
export int mm = 44;
  )cpp");
    EXPECT_FALSE(NInfo->canReuse(*Invocation, FS.view(TestDir)));

    NInfo = Builder.buildPrerequisiteModulesFor(getFullPath("N.cppm"), FS);
    EXPECT_TRUE(NInfo->canReuse(*Invocation, FS.view(TestDir)));

    CDB.addFile("foo.h", R"cpp(
inline void foo() {}
inline void foo(int) {}
  )cpp");
    EXPECT_FALSE(NInfo->canReuse(*Invocation, FS.view(TestDir)));

    NInfo = Builder.buildPrerequisiteModulesFor(getFullPath("N.cppm"), FS);
    EXPECT_TRUE(NInfo->canReuse(*Invocation, FS.view(TestDir)));
  }

  CDB.addFile("N-part.cppm", R"cpp(
export module N:Part;
// Intentioned to make it uncompilable.
export int NPart = 4LIdjwldijaw
  )cpp");
  EXPECT_FALSE(NInfo->canReuse(*Invocation, FS.view(TestDir)));
  NInfo = Builder.buildPrerequisiteModulesFor(getFullPath("N.cppm"), FS);
  EXPECT_TRUE(NInfo);
  EXPECT_FALSE(NInfo->canReuse(*Invocation, FS.view(TestDir)));

  CDB.addFile("N-part.cppm", R"cpp(
export module N:Part;
export int NPart = 43;
  )cpp");
  EXPECT_TRUE(NInfo);
  EXPECT_FALSE(NInfo->canReuse(*Invocation, FS.view(TestDir)));
  NInfo = Builder.buildPrerequisiteModulesFor(getFullPath("N.cppm"), FS);
  EXPECT_TRUE(NInfo);
  EXPECT_TRUE(NInfo->canReuse(*Invocation, FS.view(TestDir)));

  // Test that if we changed the modification time of the file, the module files
  // info is still reusable if its content doesn't change.
  CDB.addFile("N-part.cppm", R"cpp(
export module N:Part;
export int NPart = 43;
  )cpp");
  EXPECT_TRUE(NInfo->canReuse(*Invocation, FS.view(TestDir)));

  CDB.addFile("N.cppm", R"cpp(
export module N;
import :Part;
import M;

export int nn = 43;
  )cpp");
  // NInfo should be reusable after we change its content.
  EXPECT_TRUE(NInfo->canReuse(*Invocation, FS.view(TestDir)));
}

// An End-to-End test for modules.
TEST_F(PrerequisiteModulesTests, ParsedASTTest) {
  MockDirectoryCompilationDatabase CDB(TestDir, FS);

  CDB.addFile("A.cppm", R"cpp(
export module A;
export void printA();
  )cpp");

  CDB.addFile("Use.cpp", R"cpp(
import A;
)cpp");

  ModulesBuilder Builder(CDB);

  ParseInputs Use = getInputs("Use.cpp", CDB);
  Use.ModulesManager = &Builder;

  std::unique_ptr<CompilerInvocation> CI =
      buildCompilerInvocation(Use, DiagConsumer);
  EXPECT_TRUE(CI);

  auto Preamble =
      buildPreamble(getFullPath("Use.cpp"), *CI, Use, /*InMemory=*/true,
                    /*Callback=*/nullptr);
  EXPECT_TRUE(Preamble);
  EXPECT_TRUE(Preamble->RequiredModules);

  auto AST = ParsedAST::build(getFullPath("Use.cpp"), Use, std::move(CI), {},
                              Preamble);
  EXPECT_TRUE(AST);

  const NamedDecl &D = findDecl(*AST, "printA");
  EXPECT_TRUE(D.isFromASTFile());
}

// An end to end test for code complete in modules
TEST_F(PrerequisiteModulesTests, CodeCompleteTest) {
  MockDirectoryCompilationDatabase CDB(TestDir, FS);

  CDB.addFile("A.cppm", R"cpp(
export module A;
export void printA();
  )cpp");

  llvm::StringLiteral UserContents = R"cpp(
import A;
void func() {
  print^
}
)cpp";

  CDB.addFile("Use.cpp", UserContents);
  Annotations Test(UserContents);

  ModulesBuilder Builder(CDB);

  ParseInputs Use = getInputs("Use.cpp", CDB);
  Use.ModulesManager = &Builder;

  std::unique_ptr<CompilerInvocation> CI =
      buildCompilerInvocation(Use, DiagConsumer);
  EXPECT_TRUE(CI);

  auto Preamble =
      buildPreamble(getFullPath("Use.cpp"), *CI, Use, /*InMemory=*/true,
                    /*Callback=*/nullptr);
  EXPECT_TRUE(Preamble);
  EXPECT_TRUE(Preamble->RequiredModules);

  auto Result = codeComplete(getFullPath("Use.cpp"), Test.point(),
                             Preamble.get(), Use, {});
  EXPECT_FALSE(Result.Completions.empty());
  EXPECT_EQ(Result.Completions[0].Name, "printA");
}

TEST_F(PrerequisiteModulesTests, SignatureHelpTest) {
  MockDirectoryCompilationDatabase CDB(TestDir, FS);

  CDB.addFile("A.cppm", R"cpp(
export module A;
export void printA(int a);
  )cpp");

  llvm::StringLiteral UserContents = R"cpp(
import A;
void func() {
  printA(^);
}
)cpp";

  CDB.addFile("Use.cpp", UserContents);
  Annotations Test(UserContents);

  ModulesBuilder Builder(CDB);

  ParseInputs Use = getInputs("Use.cpp", CDB);
  Use.ModulesManager = &Builder;

  std::unique_ptr<CompilerInvocation> CI =
      buildCompilerInvocation(Use, DiagConsumer);
  EXPECT_TRUE(CI);

  auto Preamble =
      buildPreamble(getFullPath("Use.cpp"), *CI, Use, /*InMemory=*/true,
                    /*Callback=*/nullptr);
  EXPECT_TRUE(Preamble);
  EXPECT_TRUE(Preamble->RequiredModules);

  auto Result = signatureHelp(getFullPath("Use.cpp"), Test.point(), *Preamble,
                              Use, MarkupKind::PlainText);
  EXPECT_FALSE(Result.signatures.empty());
  EXPECT_EQ(Result.signatures[0].label, "printA(int a) -> void");
  EXPECT_EQ(Result.signatures[0].parameters[0].labelString, "int a");
}

TEST_F(PrerequisiteModulesTests, ReusablePrerequisiteModulesTest) {
  MockDirectoryCompilationDatabase CDB(TestDir, FS);

  CDB.addFile("M.cppm", R"cpp(
export module M;
export int M = 43;
  )cpp");
  CDB.addFile("A.cppm", R"cpp(
export module A;
import M;
export int A = 43 + M;
  )cpp");
  CDB.addFile("B.cppm", R"cpp(
export module B;
import M;
export int B = 44 + M;
  )cpp");

  ModulesBuilder Builder(CDB);

  auto AInfo = Builder.buildPrerequisiteModulesFor(getFullPath("A.cppm"), FS);
  EXPECT_TRUE(AInfo);
  auto BInfo = Builder.buildPrerequisiteModulesFor(getFullPath("B.cppm"), FS);
  EXPECT_TRUE(BInfo);
  HeaderSearchOptions HSOptsA(TestDir);
  HeaderSearchOptions HSOptsB(TestDir);
  AInfo->adjustHeaderSearchOptions(HSOptsA);
  BInfo->adjustHeaderSearchOptions(HSOptsB);

  EXPECT_FALSE(HSOptsA.PrebuiltModuleFiles.empty());
  EXPECT_FALSE(HSOptsB.PrebuiltModuleFiles.empty());

  // Check that we're reusing the module files.
  EXPECT_EQ(HSOptsA.PrebuiltModuleFiles, HSOptsB.PrebuiltModuleFiles);

  // Update M.cppm to check if the modules builder can update correctly.
  CDB.addFile("M.cppm", R"cpp(
export module M;
export constexpr int M = 43;
  )cpp");

  ParseInputs AUse = getInputs("A.cppm", CDB);
  AUse.ModulesManager = &Builder;
  std::unique_ptr<CompilerInvocation> AInvocation =
      buildCompilerInvocation(AUse, DiagConsumer);
  EXPECT_FALSE(AInfo->canReuse(*AInvocation, FS.view(TestDir)));

  ParseInputs BUse = getInputs("B.cppm", CDB);
  AUse.ModulesManager = &Builder;
  std::unique_ptr<CompilerInvocation> BInvocation =
      buildCompilerInvocation(BUse, DiagConsumer);
  EXPECT_FALSE(BInfo->canReuse(*BInvocation, FS.view(TestDir)));

  auto NewAInfo =
      Builder.buildPrerequisiteModulesFor(getFullPath("A.cppm"), FS);
  auto NewBInfo =
      Builder.buildPrerequisiteModulesFor(getFullPath("B.cppm"), FS);
  EXPECT_TRUE(NewAInfo);
  EXPECT_TRUE(NewBInfo);
  HeaderSearchOptions NewHSOptsA(TestDir);
  HeaderSearchOptions NewHSOptsB(TestDir);
  NewAInfo->adjustHeaderSearchOptions(NewHSOptsA);
  NewBInfo->adjustHeaderSearchOptions(NewHSOptsB);

  EXPECT_FALSE(NewHSOptsA.PrebuiltModuleFiles.empty());
  EXPECT_FALSE(NewHSOptsB.PrebuiltModuleFiles.empty());

  EXPECT_EQ(NewHSOptsA.PrebuiltModuleFiles, NewHSOptsB.PrebuiltModuleFiles);
  // Check that we didn't reuse the old and stale module files.
  EXPECT_NE(NewHSOptsA.PrebuiltModuleFiles, HSOptsA.PrebuiltModuleFiles);
}

TEST_F(PrerequisiteModulesTests, ScanningCacheTest) {
  MockDirectoryCompilationDatabase CDB(TestDir, FS);

  CDB.addFile("M.cppm", R"cpp(
export module M;
  )cpp");
  CDB.addFile("A.cppm", R"cpp(
export module A;
import M;
  )cpp");
  CDB.addFile("B.cppm", R"cpp(
export module B;
import M;
  )cpp");

  ModulesBuilder Builder(CDB);

  Builder.buildPrerequisiteModulesFor(getFullPath("A.cppm"), FS);
  Builder.buildPrerequisiteModulesFor(getFullPath("B.cppm"), FS);
  EXPECT_EQ(CDB.getGlobalScanningCount(), 1u);
}

TEST_F(PrerequisiteModulesTests, PrebuiltModuleFileTest) {
  MockDirectoryCompilationDatabase CDB(TestDir, FS);

  CDB.addFile("M.cppm", R"cpp(
export module M;
  )cpp");

  CDB.addFile("U.cpp", R"cpp(
import M;
  )cpp");

  // Use ModulesBuilder to produce the prebuilt module file.
  ModulesBuilder Builder(CDB);
  auto ModuleInfo =
      Builder.buildPrerequisiteModulesFor(getFullPath("U.cpp"), FS);
  HeaderSearchOptions HS(TestDir);
  ModuleInfo->adjustHeaderSearchOptions(HS);

  CDB.ExtraClangFlags.push_back("-fmodule-file=M=" +
                                HS.PrebuiltModuleFiles["M"]);
  ModulesBuilder Builder2(CDB);
  auto ModuleInfo2 =
      Builder2.buildPrerequisiteModulesFor(getFullPath("U.cpp"), FS);
  HeaderSearchOptions HS2(TestDir);
  ModuleInfo2->adjustHeaderSearchOptions(HS2);

  EXPECT_EQ(HS.PrebuiltModuleFiles, HS2.PrebuiltModuleFiles);
}

} // namespace
} // namespace clang::clangd

#endif
