//===----------------- ModulesBuilder.cpp ------------------------*- C++-*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ModulesBuilder.h"
#include "Compiler.h"
#include "support/Logger.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Serialization/ASTReader.h"
#include "clang/Serialization/ModuleCache.h"
#include "llvm/ADT/ScopeExit.h"
#include "llvm/Support/CommandLine.h"

#include <queue>

namespace clang {
namespace clangd {

namespace {

llvm::cl::opt<bool> DebugModulesBuilder(
    "debug-modules-builder",
    llvm::cl::desc("Don't remove clangd's built module files for debugging. "
                   "Remember to remove them later after debugging."),
    llvm::cl::init(false));

// Create a path to store module files. Generally it should be:
//
//   {TEMP_DIRS}/clangd/module_files/{hashed-file-name}-%%-%%-%%-%%-%%-%%/.
//
// {TEMP_DIRS} is the temporary directory for the system, e.g., "/var/tmp"
// or "C:/TEMP".
//
// '%%' means random value to make the generated path unique.
//
// \param MainFile is used to get the root of the project from global
// compilation database.
//
// TODO: Move these module fils out of the temporary directory if the module
// files are persistent.
llvm::SmallString<256> getUniqueModuleFilesPath(PathRef MainFile) {
  llvm::SmallString<128> HashedPrefix = llvm::sys::path::filename(MainFile);
  // There might be multiple files with the same name in a project. So appending
  // the hash value of the full path to make sure they won't conflict.
  HashedPrefix += std::to_string(llvm::hash_value(MainFile));

  llvm::SmallString<256> ResultPattern;

  llvm::sys::path::system_temp_directory(/*erasedOnReboot=*/true,
                                         ResultPattern);

  llvm::sys::path::append(ResultPattern, "clangd");
  llvm::sys::path::append(ResultPattern, "module_files");

  llvm::sys::path::append(ResultPattern, HashedPrefix);

  ResultPattern.append("-%%-%%-%%-%%-%%-%%");

  llvm::SmallString<256> Result;
  llvm::sys::fs::createUniquePath(ResultPattern, Result,
                                  /*MakeAbsolute=*/false);

  llvm::sys::fs::create_directories(Result);
  return Result;
}

// Get a unique module file path under \param ModuleFilesPrefix.
std::string getModuleFilePath(llvm::StringRef ModuleName,
                              PathRef ModuleFilesPrefix) {
  llvm::SmallString<256> ModuleFilePath(ModuleFilesPrefix);
  auto [PrimaryModuleName, PartitionName] = ModuleName.split(':');
  llvm::sys::path::append(ModuleFilePath, PrimaryModuleName);
  if (!PartitionName.empty()) {
    ModuleFilePath.append("-");
    ModuleFilePath.append(PartitionName);
  }

  ModuleFilePath.append(".pcm");
  return std::string(ModuleFilePath);
}

// FailedPrerequisiteModules - stands for the PrerequisiteModules which has
// errors happened during the building process.
class FailedPrerequisiteModules : public PrerequisiteModules {
public:
  ~FailedPrerequisiteModules() override = default;

  // We shouldn't adjust the compilation commands based on
  // FailedPrerequisiteModules.
  void adjustHeaderSearchOptions(HeaderSearchOptions &Options) const override {
  }

  // FailedPrerequisiteModules can never be reused.
  bool
  canReuse(const CompilerInvocation &CI,
           llvm::IntrusiveRefCntPtr<llvm::vfs::FileSystem>) const override {
    return false;
  }
};

/// Represents a reference to a module file (*.pcm).
class ModuleFile {
protected:
  ModuleFile(StringRef ModuleName, PathRef ModuleFilePath)
      : ModuleName(ModuleName.str()), ModuleFilePath(ModuleFilePath.str()) {}

public:
  ModuleFile() = delete;

  ModuleFile(const ModuleFile &) = delete;
  ModuleFile operator=(const ModuleFile &) = delete;

  // The move constructor is needed for llvm::SmallVector.
  ModuleFile(ModuleFile &&Other)
      : ModuleName(std::move(Other.ModuleName)),
        ModuleFilePath(std::move(Other.ModuleFilePath)) {
    Other.ModuleName.clear();
    Other.ModuleFilePath.clear();
  }

  ModuleFile &operator=(ModuleFile &&Other) {
    if (this == &Other)
      return *this;

    this->~ModuleFile();
    new (this) ModuleFile(std::move(Other));
    return *this;
  }
  virtual ~ModuleFile() = default;

  StringRef getModuleName() const { return ModuleName; }

  StringRef getModuleFilePath() const { return ModuleFilePath; }

protected:
  std::string ModuleName;
  std::string ModuleFilePath;
};

/// Represents a prebuilt module file which is not owned by us.
class PrebuiltModuleFile : public ModuleFile {
private:
  // private class to make sure the class can only be constructed by member
  // functions.
  struct CtorTag {};

public:
  PrebuiltModuleFile(StringRef ModuleName, PathRef ModuleFilePath, CtorTag)
      : ModuleFile(ModuleName, ModuleFilePath) {}

  static std::shared_ptr<PrebuiltModuleFile> make(StringRef ModuleName,
                                                  PathRef ModuleFilePath) {
    return std::make_shared<PrebuiltModuleFile>(ModuleName, ModuleFilePath,
                                                CtorTag{});
  }
};

/// Represents a module file built by us. We're responsible to remove it.
class BuiltModuleFile : public ModuleFile {
private:
  // private class to make sure the class can only be constructed by member
  // functions.
  struct CtorTag {};

public:
  BuiltModuleFile(StringRef ModuleName, PathRef ModuleFilePath, CtorTag)
      : ModuleFile(ModuleName, ModuleFilePath) {}

  static std::shared_ptr<BuiltModuleFile> make(StringRef ModuleName,
                                               PathRef ModuleFilePath) {
    return std::make_shared<BuiltModuleFile>(ModuleName, ModuleFilePath,
                                             CtorTag{});
  }

  virtual ~BuiltModuleFile() {
    if (!ModuleFilePath.empty() && !DebugModulesBuilder)
      llvm::sys::fs::remove(ModuleFilePath);
  }
};

// ReusablePrerequisiteModules - stands for PrerequisiteModules for which all
// the required modules are built successfully. All the module files
// are owned by the modules builder.
class ReusablePrerequisiteModules : public PrerequisiteModules {
public:
  ReusablePrerequisiteModules() = default;

  ReusablePrerequisiteModules(const ReusablePrerequisiteModules &Other) =
      default;
  ReusablePrerequisiteModules &
  operator=(const ReusablePrerequisiteModules &) = default;
  ReusablePrerequisiteModules(ReusablePrerequisiteModules &&) = delete;
  ReusablePrerequisiteModules
  operator=(ReusablePrerequisiteModules &&) = delete;

  ~ReusablePrerequisiteModules() override = default;

  void adjustHeaderSearchOptions(HeaderSearchOptions &Options) const override {
    // The module files that the compile command names and that cannot be used
    // would make the file fail to load them (a fatal error that ends the
    // parse), where an import that finds no module file is an ordinary error.
    for (const std::string &Name : StaleModuleNames)
      Options.PrebuiltModuleFiles.erase(Name);
    // Appending all built module files.
    for (const auto &RequiredModule : RequiredModules)
      Options.PrebuiltModuleFiles.insert_or_assign(
          RequiredModule->getModuleName().str(),
          RequiredModule->getModuleFilePath().str());
  }

  std::string getAsString() const {
    std::string Result;
    llvm::raw_string_ostream OS(Result);
    for (const auto &MF : RequiredModules) {
      OS << "-fmodule-file=" << MF->getModuleName() << "="
         << MF->getModuleFilePath() << " ";
    }
    return Result;
  }

  bool canReuse(const CompilerInvocation &CI,
                llvm::IntrusiveRefCntPtr<llvm::vfs::FileSystem>) const override;

  bool isModuleUnitBuilt(llvm::StringRef ModuleName) const {
    return BuiltModuleNames.contains(ModuleName);
  }

  void addModuleFile(std::shared_ptr<const ModuleFile> MF) {
    BuiltModuleNames.insert(MF->getModuleName());
    RequiredModules.emplace_back(std::move(MF));
  }

  /// Names of module files in the compile command that must not be used.
  void setStaleModuleNames(std::vector<std::string> Names) {
    StaleModuleNames = std::move(Names);
  }

private:
  llvm::SmallVector<std::shared_ptr<const ModuleFile>, 8> RequiredModules;
  std::vector<std::string> StaleModuleNames;
  // A helper class to speedup the query if a module is built.
  llvm::StringSet<> BuiltModuleNames;
};

// PartialPrerequisiteModules - the module files that could be built when some of
// the required modules could not. They are used so that the file sees what
// exists, but the file is never considered up to date: the next update builds
// the preamble again, which finds a module that exists by then.
class PartialPrerequisiteModules : public PrerequisiteModules {
public:
  explicit PartialPrerequisiteModules(const ReusablePrerequisiteModules &Built)
      : Built(Built) {}

  void adjustHeaderSearchOptions(HeaderSearchOptions &Options) const override {
    Built.adjustHeaderSearchOptions(Options);
  }

  bool canReuse(const CompilerInvocation &CI,
                llvm::IntrusiveRefCntPtr<llvm::vfs::FileSystem>) const override {
    return false;
  }

private:
  ReusablePrerequisiteModules Built;
};

// BuildSystemPrerequisiteModules - the module files that the build system gave
// to the compiler (-fmodule-file=) and that were found usable: they exist, the
// compiler can read them and none of their inputs changed. clangd uses them as
// they are (the compile command already names them), but it keeps checking that
// they stay usable, since the build system may rebuild them or the user may edit
// the module units they were built from. A stale or missing module file is not
// trusted: clangd scans the project and builds its own module files then.
class BuildSystemPrerequisiteModules : public PrerequisiteModules {
public:
  /// Name is empty for -fmodule-file=<path>; Path is absolute.
  struct ModuleFileRef {
    std::string Name;
    std::string Path;
  };

  explicit BuildSystemPrerequisiteModules(std::vector<ModuleFileRef> ModuleFiles)
      : ModuleFiles(std::move(ModuleFiles)) {}

  void adjustHeaderSearchOptions(HeaderSearchOptions &Options) const override {
    // The compile command names them already (possibly relative to the working
    // directory); this makes the paths absolute.
    for (const ModuleFileRef &MF : ModuleFiles)
      if (!MF.Name.empty())
        Options.PrebuiltModuleFiles.insert_or_assign(MF.Name, MF.Path);
  }

  bool canReuse(const CompilerInvocation &CI,
                llvm::IntrusiveRefCntPtr<llvm::vfs::FileSystem> VFS)
      const override;

private:
  std::vector<ModuleFileRef> ModuleFiles;
};

bool IsModuleFileUpToDate(PathRef ModuleFilePath,
                          const PrerequisiteModules &RequisiteModules,
                          llvm::IntrusiveRefCntPtr<llvm::vfs::FileSystem> VFS) {
  HeaderSearchOptions HSOpts;
  RequisiteModules.adjustHeaderSearchOptions(HSOpts);
  HSOpts.ForceCheckCXX20ModulesInputFiles = true;
  HSOpts.ValidateASTInputFilesContent = true;

  clang::clangd::IgnoreDiagnostics IgnoreDiags;
  DiagnosticOptions DiagOpts;
  IntrusiveRefCntPtr<DiagnosticsEngine> Diags =
      CompilerInstance::createDiagnostics(*VFS, DiagOpts, &IgnoreDiags,
                                          /*ShouldOwnClient=*/false);

  LangOptions LangOpts;
  LangOpts.SkipODRCheckInGMF = true;

  FileManager FileMgr(FileSystemOptions(), VFS);

  SourceManager SourceMgr(*Diags, FileMgr);

  HeaderSearch HeaderInfo(HSOpts, SourceMgr, *Diags, LangOpts,
                          /*Target=*/nullptr);

  PreprocessorOptions PPOpts;
  TrivialModuleLoader ModuleLoader;
  Preprocessor PP(PPOpts, *Diags, LangOpts, SourceMgr, HeaderInfo,
                  ModuleLoader);

  std::shared_ptr<ModuleCache> ModCache = createCrossProcessModuleCache();
  PCHContainerOperations PCHOperations;
  CodeGenOptions CodeGenOpts;
  // The reader compares the content of the input files with the hashes in the
  // module file only if it is asked to (HSOpts.ValidateASTInputFilesContent
  // alone is not enough): the size and the modification time, which is in
  // seconds, do not tell an edit that keeps the size within the same second.
  ASTReader Reader(PP, *ModCache, /*ASTContext=*/nullptr,
                   PCHOperations.getRawReader(), CodeGenOpts, {},
                   /*isysroot=*/"", DisableValidationForModuleKind::None,
                   /*AllowASTWithCompilerErrors=*/false,
                   /*AllowConfigurationMismatch=*/false,
                   /*ValidateSystemInputs=*/false,
                   /*ForceValidateUserInputs=*/true,
                   /*ValidateASTInputFilesContent=*/true);

  // We don't need any listener here. By default it will use a validator
  // listener.
  Reader.setListener(nullptr);

  if (auto Result = Reader.ReadAST(ModuleFilePath, serialization::MK_MainFile,
                                   SourceLocation(), ASTReader::ARR_None);
      Result != ASTReader::Success) {
    vlog("Module file {0} cannot be read (reader result {1})", ModuleFilePath,
         static_cast<int>(Result));
    return false;
  }

  bool UpToDate = true;
  Reader.getModuleManager().visit([&](serialization::ModuleFile &MF) -> bool {
    Reader.visitInputFiles(
        MF, /*IncludeSystem=*/false, /*Complain=*/false,
        [&](const serialization::InputFile &IF, bool isSystem) {
          if (!IF.getFile() || IF.isOutOfDate()) {
            vlog("Module file {0} is out of date: input {1} of {2}",
                 ModuleFilePath,
                 IF.getFile() ? IF.getFile()->getName() : StringRef("<missing>"),
                 MF.FileName);
            UpToDate = false;
          }
        });
    return !UpToDate;
  });
  return UpToDate;
}

bool IsModuleFilesUpToDate(
    llvm::SmallVector<PathRef> ModuleFilePaths,
    const PrerequisiteModules &RequisiteModules,
    llvm::IntrusiveRefCntPtr<llvm::vfs::FileSystem> VFS) {
  return llvm::all_of(
      ModuleFilePaths, [&RequisiteModules, VFS](auto ModuleFilePath) {
        return IsModuleFileUpToDate(ModuleFilePath, RequisiteModules, VFS);
      });
}

/// Build a module file for module with `ModuleName`. The information of built
/// module file are stored in \param BuiltModuleFiles.
llvm::Expected<std::shared_ptr<BuiltModuleFile>>
buildModuleFile(llvm::StringRef ModuleName, PathRef ModuleUnitFileName,
                const GlobalCompilationDatabase &CDB, const ThreadsafeFS &TFS,
                const ReusablePrerequisiteModules &BuiltModuleFiles,
                std::optional<tooling::CompileCommand> CommandOverride =
                    std::nullopt) {
  // Try cheap operation earlier to boil-out cheaply if there are problems.
  auto Cmd = CommandOverride ? std::move(CommandOverride)
                             : CDB.getCompileCommand(ModuleUnitFileName);
  if (!Cmd)
    return llvm::createStringError(
        llvm::formatv("No compile command for {0}", ModuleUnitFileName));

  llvm::SmallString<256> ModuleFilesPrefix =
      getUniqueModuleFilesPath(ModuleUnitFileName);

  Cmd->Output = getModuleFilePath(ModuleName, ModuleFilesPrefix);

  ParseInputs Inputs;
  Inputs.TFS = &TFS;
  Inputs.CompileCommand = std::move(*Cmd);

  IgnoreDiagnostics IgnoreDiags;
  auto CI = buildCompilerInvocation(Inputs, IgnoreDiags);
  if (!CI)
    return llvm::createStringError("Failed to build compiler invocation");

  auto FS = Inputs.TFS->view(Inputs.CompileCommand.Directory);
  auto Buf = FS->getBufferForFile(Inputs.CompileCommand.Filename);
  if (!Buf)
    return llvm::createStringError(
        llvm::formatv("Failed to create buffer for {0} (in {1}): {2}",
                      Inputs.CompileCommand.Filename,
                      Inputs.CompileCommand.Directory,
                      Buf.getError().message()));

  // In clang's driver, we will suppress the check for ODR violation in GMF.
  // See the implementation of RenderModulesOptions in Clang.cpp.
  CI->getLangOpts().SkipODRCheckInGMF = true;

  // Hash the contents of input files and store the hash value to the BMI files.
  // So that we can check if the files are still valid when we want to reuse the
  // BMI files.
  CI->getHeaderSearchOpts().ValidateASTInputFilesContent = true;

  BuiltModuleFiles.adjustHeaderSearchOptions(CI->getHeaderSearchOpts());

  CI->getFrontendOpts().OutputFile = Inputs.CompileCommand.Output;
  auto Clang =
      prepareCompilerInstance(std::move(CI), /*Preamble=*/nullptr,
                              std::move(*Buf), std::move(FS), IgnoreDiags);
  if (!Clang)
    return llvm::createStringError("Failed to prepare compiler instance");

  GenerateReducedModuleInterfaceAction Action;
  Clang->ExecuteAction(Action);

  if (Clang->getDiagnostics().hasErrorOccurred()) {
    std::string Cmds;
    for (const auto &Arg : Inputs.CompileCommand.CommandLine) {
      if (!Cmds.empty())
        Cmds += " ";
      Cmds += Arg;
    }

    clangd::vlog("Failed to compile {0} with command: {1}", ModuleUnitFileName,
                 Cmds);

    std::string BuiltModuleFilesStr = BuiltModuleFiles.getAsString();
    if (!BuiltModuleFilesStr.empty())
      clangd::vlog("The actual used module files built by clangd is {0}",
                   BuiltModuleFilesStr);

    return llvm::createStringError(
        llvm::formatv("Failed to compile {0}. Use '--log=verbose' to view "
                      "detailed failure reasons. It is helpful to use "
                      "'--debug-modules-builder' flag to keep the clangd's "
                      "built module files to reproduce the failure for "
                      "debugging. Remember to remove them after debugging.",
                      ModuleUnitFileName));
  }

  return BuiltModuleFile::make(ModuleName, Inputs.CompileCommand.Output);
}

bool ReusablePrerequisiteModules::canReuse(
    const CompilerInvocation &CI,
    llvm::IntrusiveRefCntPtr<llvm::vfs::FileSystem> VFS) const {
  if (RequiredModules.empty())
    return true;

  llvm::SmallVector<llvm::StringRef> BMIPaths;
  for (auto &MF : RequiredModules)
    BMIPaths.push_back(MF->getModuleFilePath());
  return IsModuleFilesUpToDate(BMIPaths, *this, VFS);
}

bool BuildSystemPrerequisiteModules::canReuse(
    const CompilerInvocation &CI,
    llvm::IntrusiveRefCntPtr<llvm::vfs::FileSystem> VFS) const {
  // The module files import one another by the paths they were built with,
  // which are relative to the directory of the build system; the absolute paths
  // that the compile command names for them are used instead.
  return llvm::all_of(ModuleFiles, [&](const ModuleFileRef &MF) {
    return IsModuleFileUpToDate(MF.Path, *this, VFS);
  });
}

/// The module files that the compile command of \param Cmd asks for with
/// -fmodule-file=, if every one of them is usable. Returns null if there is
/// none or if one of them is not usable.
std::unique_ptr<BuildSystemPrerequisiteModules>
getUsableBuildSystemModules(const tooling::CompileCommand &Cmd,
                            const ThreadsafeFS &TFS) {
  ParseInputs Inputs;
  Inputs.TFS = &TFS;
  Inputs.CompileCommand = Cmd;
  IgnoreDiagnostics IgnoreDiags;
  auto CI = buildCompilerInvocation(Inputs, IgnoreDiags);
  if (!CI)
    return nullptr;

  std::vector<BuildSystemPrerequisiteModules::ModuleFileRef> Paths;
  auto Add = [&](llvm::StringRef Name, llvm::StringRef Path) {
    llvm::SmallString<256> Abs(Path);
    llvm::sys::path::make_absolute(Cmd.Directory, Abs);
    llvm::sys::path::remove_dots(Abs, /*remove_dot_dot=*/true);
    Paths.push_back({Name.str(), std::string(Abs)});
  };
  for (const auto &[Name, Path] : CI->getHeaderSearchOpts().PrebuiltModuleFiles)
    Add(Name, Path);
  for (const std::string &Path : CI->getFrontendOpts().ModuleFiles)
    Add("", Path);
  if (Paths.empty())
    return nullptr;

  auto Result = std::make_unique<BuildSystemPrerequisiteModules>(std::move(Paths));
  if (!Result->canReuse(*CI, TFS.view(Cmd.Directory))) {
    log("The module files that the build system gave for {0} are missing or "
        "out of date; clangd builds its own",
        Cmd.Filename);
    return nullptr;
  }
  return Result;
}

/// The names of the module files in the compile command of \param Cmd that are
/// not usable (see getUsableBuildSystemModules).
std::vector<std::string>
getUnusableBuildSystemModuleNames(const tooling::CompileCommand &Cmd,
                                  const ThreadsafeFS &TFS) {
  ParseInputs Inputs;
  Inputs.TFS = &TFS;
  Inputs.CompileCommand = Cmd;
  IgnoreDiagnostics IgnoreDiags;
  auto CI = buildCompilerInvocation(Inputs, IgnoreDiags);
  std::vector<std::string> Names;
  if (!CI)
    return Names;
  std::vector<BuildSystemPrerequisiteModules::ModuleFileRef> Refs;
  for (const auto &[Name, Path] : CI->getHeaderSearchOpts().PrebuiltModuleFiles) {
    llvm::SmallString<256> Abs(Path);
    llvm::sys::path::make_absolute(Cmd.Directory, Abs);
    llvm::sys::path::remove_dots(Abs, /*remove_dot_dot=*/true);
    Refs.push_back({Name, std::string(Abs)});
  }
  BuildSystemPrerequisiteModules All(Refs);
  for (const auto &Ref : Refs)
    if (!IsModuleFileUpToDate(Ref.Path, All, TFS.view(Cmd.Directory)))
      Names.push_back(Ref.Name);
  return Names;
}

class ModuleFileCache {
public:
  ModuleFileCache(const GlobalCompilationDatabase &CDB) : CDB(CDB) {}
  const GlobalCompilationDatabase &getCDB() const { return CDB; }

  std::shared_ptr<const ModuleFile> getModule(StringRef ModuleName);

  void add(StringRef ModuleName, std::shared_ptr<const ModuleFile> ModuleFile) {
    std::lock_guard<std::mutex> Lock(ModuleFilesMutex);

    ModuleFiles[ModuleName] = ModuleFile;
  }

  void remove(StringRef ModuleName);

private:
  const GlobalCompilationDatabase &CDB;

  llvm::StringMap<std::weak_ptr<const ModuleFile>> ModuleFiles;
  // Mutex to guard accesses to ModuleFiles.
  std::mutex ModuleFilesMutex;
};

std::shared_ptr<const ModuleFile>
ModuleFileCache::getModule(StringRef ModuleName) {
  std::lock_guard<std::mutex> Lock(ModuleFilesMutex);

  auto Iter = ModuleFiles.find(ModuleName);
  if (Iter == ModuleFiles.end())
    return nullptr;

  if (auto Res = Iter->second.lock())
    return Res;

  ModuleFiles.erase(Iter);
  return nullptr;
}

void ModuleFileCache::remove(StringRef ModuleName) {
  std::lock_guard<std::mutex> Lock(ModuleFilesMutex);

  ModuleFiles.erase(ModuleName);
}

class ModuleNameToSourceCache {
public:
  std::string getSourceForModuleName(llvm::StringRef ModuleName) {
    std::lock_guard<std::mutex> Lock(CacheMutex);
    auto Iter = ModuleNameToSourceCache.find(ModuleName);
    if (Iter != ModuleNameToSourceCache.end())
      return Iter->second;
    return "";
  }

  void addEntry(llvm::StringRef ModuleName, PathRef Source) {
    std::lock_guard<std::mutex> Lock(CacheMutex);
    ModuleNameToSourceCache[ModuleName] = Source.str();
  }

  void eraseEntry(llvm::StringRef ModuleName) {
    std::lock_guard<std::mutex> Lock(CacheMutex);
    ModuleNameToSourceCache.erase(ModuleName);
  }

private:
  std::mutex CacheMutex;
  llvm::StringMap<std::string> ModuleNameToSourceCache;
};

class CachingProjectModules : public ProjectModules {
public:
  CachingProjectModules(std::unique_ptr<ProjectModules> MDB,
                        ModuleNameToSourceCache &Cache)
      : MDB(std::move(MDB)), Cache(Cache) {
    assert(this->MDB && "CachingProjectModules should only be created with a "
                        "valid underlying ProjectModules");
  }

  std::vector<std::string> getRequiredModules(PathRef File) override {
    return MDB->getRequiredModules(File);
  }

  std::string getModuleNameForSource(PathRef File) override {
    return MDB->getModuleNameForSource(File);
  }

  std::optional<tooling::CompileCommand>
  getCompileCommandForSource(PathRef File) override {
    return MDB->getCompileCommandForSource(File);
  }

  std::string getSourceForModuleName(llvm::StringRef ModuleName,
                                     PathRef RequiredSrcFile) override {
    std::string CachedResult = Cache.getSourceForModuleName(ModuleName);

    // Verify Cached Result by seeing if the source declaring the same module
    // as we query.
    if (!CachedResult.empty()) {
      std::string ModuleNameOfCachedSource =
          MDB->getModuleNameForSource(CachedResult);
      if (ModuleNameOfCachedSource == ModuleName)
        return CachedResult;

      // Cached Result is invalid. Clear it.
      Cache.eraseEntry(ModuleName);
    }

    auto Result = MDB->getSourceForModuleName(ModuleName, RequiredSrcFile);
    Cache.addEntry(ModuleName, Result);

    return Result;
  }

private:
  std::unique_ptr<ProjectModules> MDB;
  ModuleNameToSourceCache &Cache;
};

/// Collect the directly and indirectly required module names for \param
/// ModuleName in topological order. The \param ModuleName is guaranteed to
/// be the last element in \param ModuleNames.
llvm::SmallVector<std::string> getAllRequiredModules(PathRef RequiredSource,
                                                     CachingProjectModules &MDB,
                                                     StringRef ModuleName) {
  llvm::SmallVector<std::string> ModuleNames;
  llvm::StringSet<> ModuleNamesSet;

  auto VisitDeps = [&](StringRef ModuleName, auto Visitor) -> void {
    ModuleNamesSet.insert(ModuleName);

    for (StringRef RequiredModuleName : MDB.getRequiredModules(
             MDB.getSourceForModuleName(ModuleName, RequiredSource)))
      if (ModuleNamesSet.insert(RequiredModuleName).second)
        Visitor(RequiredModuleName, Visitor);

    ModuleNames.push_back(ModuleName.str());
  };
  VisitDeps(ModuleName, VisitDeps);

  return ModuleNames;
}

} // namespace

class ModulesBuilder::ModulesBuilderImpl {
public:
  ModulesBuilderImpl(const GlobalCompilationDatabase &CDB) : Cache(CDB) {}

  ModuleNameToSourceCache &getProjectModulesCache() {
    return ProjectModulesCache;
  }
  const GlobalCompilationDatabase &getCDB() const { return Cache.getCDB(); }

  llvm::Error
  getOrBuildModuleFile(PathRef RequiredSource, StringRef ModuleName,
                       const ThreadsafeFS &TFS, CachingProjectModules &MDB,
                       ReusablePrerequisiteModules &BuiltModuleFiles);

private:
  /// Try to get prebuilt module files from the compilation database.
  void getPrebuiltModuleFile(StringRef ModuleName, PathRef ModuleUnitFileName,
                             const ThreadsafeFS &TFS,
                             ReusablePrerequisiteModules &BuiltModuleFiles);

  ModuleFileCache Cache;
  ModuleNameToSourceCache ProjectModulesCache;
};

void ModulesBuilder::ModulesBuilderImpl::getPrebuiltModuleFile(
    StringRef ModuleName, PathRef ModuleUnitFileName, const ThreadsafeFS &TFS,
    ReusablePrerequisiteModules &BuiltModuleFiles) {
  auto Cmd = getCDB().getCompileCommand(ModuleUnitFileName);
  if (!Cmd)
    return;

  ParseInputs Inputs;
  Inputs.TFS = &TFS;
  Inputs.CompileCommand = std::move(*Cmd);

  IgnoreDiagnostics IgnoreDiags;
  auto CI = buildCompilerInvocation(Inputs, IgnoreDiags);
  if (!CI)
    return;

  // We don't need to check if the module files are in ModuleCache or adding
  // them to the module cache. As even if the module files are in the module
  // cache, we still need to validate them. And it looks not helpful to add them
  // to the module cache, since we may always try to get the prebuilt module
  // files before building the module files by ourselves.
  for (auto &[ModuleName, ModuleFilePath] :
       CI->getHeaderSearchOpts().PrebuiltModuleFiles) {
    if (BuiltModuleFiles.isModuleUnitBuilt(ModuleName))
      continue;

    llvm::SmallString<256> AbsoluteModuleFilePath(ModuleFilePath);
    llvm::sys::path::make_absolute(Inputs.CompileCommand.Directory,
                                   AbsoluteModuleFilePath);
    if (IsModuleFileUpToDate(AbsoluteModuleFilePath, BuiltModuleFiles,
                             TFS.view(Inputs.CompileCommand.Directory))) {
      log("Reusing prebuilt module file {0} of module {1} for {2}",
          ModuleFilePath, ModuleName, ModuleUnitFileName);
      BuiltModuleFiles.addModuleFile(
          PrebuiltModuleFile::make(ModuleName, AbsoluteModuleFilePath));
    }
  }
}

llvm::Error ModulesBuilder::ModulesBuilderImpl::getOrBuildModuleFile(
    PathRef RequiredSource, StringRef ModuleName, const ThreadsafeFS &TFS,
    CachingProjectModules &MDB, ReusablePrerequisiteModules &BuiltModuleFiles) {
  if (BuiltModuleFiles.isModuleUnitBuilt(ModuleName))
    return llvm::Error::success();

  std::string ModuleUnitFileName =
      MDB.getSourceForModuleName(ModuleName, RequiredSource);
  /// It is possible that we're meeting third party modules (modules whose
  /// source are not in the project. e.g, the std module may be a third-party
  /// module for most project) or something wrong with the implementation of
  /// ProjectModules.
  /// FIXME: How should we treat third party modules here? If we want to ignore
  /// third party modules, we should return true instead of false here.
  /// Currently we simply bail out.
  if (ModuleUnitFileName.empty())
    return llvm::createStringError(
        llvm::formatv("Don't get the module unit for module {0}", ModuleName));

  /// Try to get prebuilt module files from the compilation database first. This
  /// helps to avoid building the module files that are already built by the
  /// compiler.
  getPrebuiltModuleFile(ModuleName, ModuleUnitFileName, TFS, BuiltModuleFiles);

  // Get Required modules in topological order.
  auto ReqModuleNames = getAllRequiredModules(RequiredSource, MDB, ModuleName);
  for (llvm::StringRef ReqModuleName : ReqModuleNames) {
    if (BuiltModuleFiles.isModuleUnitBuilt(ReqModuleName))
      continue;

    if (auto Cached = Cache.getModule(ReqModuleName)) {
      if (IsModuleFileUpToDate(Cached->getModuleFilePath(), BuiltModuleFiles,
                               TFS.view(std::nullopt))) {
        log("Reusing module {0} from {1}", ReqModuleName,
            Cached->getModuleFilePath());
        BuiltModuleFiles.addModuleFile(std::move(Cached));
        continue;
      }
      Cache.remove(ReqModuleName);
    }

    std::string ReqFileName =
        MDB.getSourceForModuleName(ReqModuleName, RequiredSource);
    llvm::Expected<std::shared_ptr<BuiltModuleFile>> MF = buildModuleFile(
        ReqModuleName, ReqFileName, getCDB(), TFS, BuiltModuleFiles,
        MDB.getCompileCommandForSource(ReqFileName));
    if (llvm::Error Err = MF.takeError())
      return Err;

    log("Built module {0} to {1}", ReqModuleName, (*MF)->getModuleFilePath());
    Cache.add(ReqModuleName, *MF);
    BuiltModuleFiles.addModuleFile(std::move(*MF));
  }

  return llvm::Error::success();
}

std::unique_ptr<PrerequisiteModules>
ModulesBuilder::buildPrerequisiteModulesFor(PathRef File,
                                            const ThreadsafeFS &TFS) {
  // Build systems may provide all module mappings directly. In that case the
  // compiler invocation already knows which BMIs to use, and rebuilding them
  // after globally scanning the project is both redundant and expensive. They
  // are only used while they are usable: a module file that is missing, was
  // written by another version of the compiler or is older than its sources
  // sends us down the scanning path below.
  std::vector<std::string> StaleModuleNames;
  if (auto Cmd = Impl->getCDB().getCompileCommand(File)) {
    if (llvm::any_of(Cmd->CommandLine, [](llvm::StringRef Arg) {
          return Arg.starts_with("-fmodule-file=");
        })) {
      if (auto Provided = getUsableBuildSystemModules(*Cmd, TFS))
        return Provided;
      StaleModuleNames = getUnusableBuildSystemModuleNames(*Cmd, TFS);
    }
  }

  std::unique_ptr<ProjectModules> MDB = Impl->getCDB().getProjectModules(File);
  if (!MDB) {
    elog("Failed to get Project Modules information for {0}", File);
    return std::make_unique<FailedPrerequisiteModules>();
  }
  CachingProjectModules CachedMDB(std::move(MDB),
                                  Impl->getProjectModulesCache());

  std::vector<std::string> RequiredModuleNames =
      CachedMDB.getRequiredModules(File);
  if (RequiredModuleNames.empty()) {
    auto None = std::make_unique<ReusablePrerequisiteModules>();
    None->setStaleModuleNames(std::move(StaleModuleNames));
    return None;
  }

  auto RequiredModules = std::make_unique<ReusablePrerequisiteModules>();
  RequiredModules->setStaleModuleNames(std::move(StaleModuleNames));
  bool AllBuilt = true;
  for (llvm::StringRef RequiredModuleName : RequiredModuleNames) {
    // A module that cannot be built does not stop the others: the file still
    // gets the module files that exist, and the diagnostics point at the
    // modules that are really missing.
    if (llvm::Error Err = Impl->getOrBuildModuleFile(
            File, RequiredModuleName, TFS, CachedMDB, *RequiredModules.get())) {
      elog("Failed to build module {0}; due to {1}", RequiredModuleName,
           toString(std::move(Err)));
      AllBuilt = false;
    }
  }

  if (!AllBuilt)
    return std::make_unique<PartialPrerequisiteModules>(*RequiredModules);
  return std::move(RequiredModules);
}

ModulesBuilder::ModulesBuilder(const GlobalCompilationDatabase &CDB) {
  Impl = std::make_unique<ModulesBuilderImpl>(CDB);
}

ModulesBuilder::~ModulesBuilder() {}

} // namespace clangd
} // namespace clang
