//===------------------ ProjectModules.h -------------------------*- C++-*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ProjectModules.h"
#include "support/Logger.h"
#include "clang/DependencyScanning/DependencyScanningService.h"
#include "clang/Tooling/DependencyScanningTool.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Program.h"
#include "llvm/Support/Regex.h"
#include <chrono>
#include <mutex>
#include <set>

namespace clang::clangd {

/// What the scans of one compilation database found, kept across requests. An
/// entry for a file is used while the file (its size and modification time) and
/// the command it is scanned with stay the same; everything else is rescanned.
class ModuleScanCache {
public:
  struct FileScan {
    llvm::sys::TimePoint<> ModificationTime;
    uint64_t Size = 0;
    size_t CommandHash = 0;
    bool Succeeded = false;
    std::optional<std::string> ModuleName;
    std::vector<std::string> RequiredModules;
  };

  std::mutex Mu;
  llvm::StringMap<FileScan> Files;
  // Map module name to source file path.
  llvm::StringMap<std::string> ModuleNameToSource;
  // Commands for module units that are not in the compilation database.
  llvm::StringMap<tooling::CompileCommand> ExtraCommands;
  // When the directories of the project were last searched for module units
  // that the database does not list.
  std::optional<std::chrono::steady_clock::time_point> LastDiscovery;
};

std::shared_ptr<ModuleScanCache> createModuleScanCache() {
  return std::make_shared<ModuleScanCache>();
}

namespace {

/// Find the module unit of a module of the standard library (std, std.compat)
/// with the manifest libc++ installs next to its libraries
/// (lib/<triple>/libc++.modules.json) and make the command to build it from the
/// command of a file that imports it: the same flags, a different input.
/// The manifest of the toolchain the command names is used if that toolchain
/// exists; a project database may name a toolchain that was replaced or removed
/// since the build system generated it, so the toolchain that clangd itself
/// belongs to is the fallback.
std::optional<tooling::CompileCommand>
resolveStandardLibraryModule(llvm::StringRef ModuleName,
                             const tooling::CompileCommand &Importer,
                             std::string &SourcePath) {
  llvm::SmallVector<std::string, 2> BinDirs;
  if (!Importer.CommandLine.empty()) {
    llvm::StringRef Compiler = Importer.CommandLine.front();
    if (llvm::sys::path::has_parent_path(Compiler))
      BinDirs.push_back(llvm::sys::path::parent_path(Compiler).str());
  }
  std::string Self = llvm::sys::fs::getMainExecutable(nullptr, nullptr);
  if (!Self.empty())
    BinDirs.push_back(llvm::sys::path::parent_path(Self).str());

  for (const std::string &BinDir : BinDirs) {
    llvm::SmallString<256> LibDir(BinDir);
    llvm::sys::path::append(LibDir, "..", "lib");
    llvm::sys::path::remove_dots(LibDir, /*remove_dot_dot=*/true);
    llvm::SmallVector<std::string, 4> Manifests;
    llvm::SmallString<256> Direct(LibDir);
    llvm::sys::path::append(Direct, "libc++.modules.json");
    Manifests.push_back(std::string(Direct));
    std::error_code EC;
    for (llvm::sys::fs::directory_iterator It(LibDir, EC), End;
         !EC && It != End; It.increment(EC)) {
      llvm::SmallString<256> InTriple(It->path());
      llvm::sys::path::append(InTriple, "libc++.modules.json");
      Manifests.push_back(std::string(InTriple));
    }
    for (const std::string &Manifest : Manifests) {
      auto Buf = llvm::MemoryBuffer::getFile(Manifest);
      if (!Buf)
        continue;
      auto Parsed = llvm::json::parse((*Buf)->getBuffer());
      if (!Parsed) {
        llvm::consumeError(Parsed.takeError());
        continue;
      }
      const llvm::json::Object *Root = Parsed->getAsObject();
      const llvm::json::Array *Modules = Root ? Root->getArray("modules") : nullptr;
      if (!Modules)
        continue;
      for (const llvm::json::Value &M : *Modules) {
        const llvm::json::Object *Obj = M.getAsObject();
        if (!Obj || Obj->getString("logical-name") != ModuleName)
          continue;
        auto Rel = Obj->getString("source-path");
        if (!Rel)
          continue;
        llvm::SmallString<256> ManifestDir(Manifest);
        llvm::sys::path::remove_filename(ManifestDir);
        llvm::SmallString<256> Source(*Rel);
        llvm::sys::path::make_absolute(ManifestDir, Source);
        llvm::sys::path::remove_dots(Source, /*remove_dot_dot=*/true);
        if (!llvm::sys::fs::exists(Source))
          continue;

        tooling::CompileCommand Out = Importer;
        Out.Filename = std::string(Source);
        Out.CommandLine.clear();
        llvm::StringRef ImporterFile = llvm::sys::path::filename(Importer.Filename);
        for (size_t I = 0; I < Importer.CommandLine.size(); ++I) {
          llvm::StringRef Arg = Importer.CommandLine[I];
          if (I > 0 && (Arg == "-o" || Arg == "--")) {
            if (Arg == "-o")
              ++I;
            continue;
          }
          if (Arg.starts_with("-fmodule-output") ||
              Arg.starts_with("-fmodule-file=std=") ||
              Arg.starts_with("-fmodule-file=std.compat="))
            continue;
          if (I > 0 && !Arg.starts_with("-") &&
              llvm::sys::path::filename(Arg) == ImporterFile)
            continue;
          Out.CommandLine.push_back(Importer.CommandLine[I]);
        }
        if (const llvm::json::Object *Local = Obj->getObject("local-arguments"))
          if (const llvm::json::Array *Dirs =
                  Local->getArray("system-include-directories"))
            for (const llvm::json::Value &D : *Dirs)
              if (auto DirStr = D.getAsString()) {
                llvm::SmallString<256> Dir(*DirStr);
                llvm::sys::path::make_absolute(ManifestDir, Dir);
                llvm::sys::path::remove_dots(Dir, /*remove_dot_dot=*/true);
                Out.CommandLine.push_back("-isystem");
                Out.CommandLine.push_back(std::string(Dir));
              }
        Out.CommandLine.push_back(std::string(Source));
        SourcePath = std::string(Source);
        return Out;
      }
    }
  }
  return std::nullopt;
}

/// A scanner to query the dependency information for C++20 Modules.
///
/// The scanner can scan a single file with `scan(PathRef)` member function
/// or scan the whole project with `globalScan(vector<PathRef>)` member
/// function. See the comments of `globalScan` to see the details.
///
/// The ModuleDependencyScanner can get the directly required module names for a
/// specific source file. Also the ModuleDependencyScanner can get the source
/// file declaring the primary module interface for a specific module name.
///
/// IMPORTANT NOTE: we assume that every module unit is only declared once in a
/// source file in the project. But the assumption is not strictly true even
/// besides the invalid projects. The language specification requires that every
/// module unit should be unique in a valid program. But a project can contain
/// multiple programs. Then it is valid that we can have multiple source files
/// declaring the same module in a project as long as these source files don't
/// interfere with each other.
class ModuleDependencyScanner {
public:
  ModuleDependencyScanner(
      std::shared_ptr<const clang::tooling::CompilationDatabase> CDB,
      const ThreadsafeFS &TFS, std::shared_ptr<ModuleScanCache> Cache)
      : CDB(CDB), TFS(TFS), Cache(std::move(Cache)),
        Service(dependencies::ScanningMode::CanonicalPreprocessing,
                dependencies::ScanningOutputFormat::P1689) {}

  /// The scanned modules dependency information for a specific source file.
  struct ModuleDependencyInfo {
    /// The name of the module if the file is a module unit.
    std::optional<std::string> ModuleName;
    /// A list of names for the modules that the file directly depends.
    std::vector<std::string> RequiredModules;
  };

  /// Scanning the single file specified by \param FilePath.
  std::optional<ModuleDependencyInfo>
  scan(PathRef FilePath, const ProjectModules::CommandMangler &Mangler);

  /// Look for module units that the compilation database does not list (a
  /// file the build system has not seen yet) in the directories that contain
  /// the files it does list, and add them to the <module-name> to
  /// <module-unit-source> map. See the comments of globalScan.
  void discoverUnlistedModules(const ProjectModules::CommandMangler &Mangler);

  /// Scanning every source file in the current project to get the
  /// <module-name> to <module-unit-source> map.
  /// TODO: We should find an efficient method to get the <module-name>
  /// to <module-unit-source> map. We can make it either by providing
  /// a global module dependency scanner to monitor every file. Or we
  /// can simply require the build systems (or even the end users)
  /// to provide the map.
  void globalScan(const ProjectModules::CommandMangler &Mangler);

  /// Get the source file from the module name. Note that the language
  /// guarantees all the module names are unique in a valid program.
  /// This function should only be called after globalScan.
  ///
  /// TODO: We should handle the case that there are multiple source files
  /// declaring the same module.
  PathRef getSourceForModuleName(llvm::StringRef ModuleName) const;

  /// Like getSourceForModuleName, but also finds the modules of the standard
  /// library, which are not part of the project, with the flags of
  /// \param ImportingFile.
  std::string getSourceForModuleName(llvm::StringRef ModuleName,
                                     PathRef ImportingFile,
                                     const ProjectModules::CommandMangler &Mangler);

  /// The command registered for a source that is not in the project's
  /// compilation database (see getSourceForModuleName).
  std::optional<tooling::CompileCommand>
  getCompileCommandForSource(PathRef File) const;

  /// Return the direct required modules. Indirect required modules are not
  /// included.
  std::vector<std::string>
  getRequiredModules(PathRef File,
                     const ProjectModules::CommandMangler &Mangler);

private:
  std::shared_ptr<const clang::tooling::CompilationDatabase> CDB;
  const ThreadsafeFS &TFS;

  // Whether the scanner has scanned the project globally.
  bool GlobalScanned = false;

  // The results of the scans of earlier requests.
  std::shared_ptr<ModuleScanCache> Cache;

  // The scanner's own view of the file system (its cache of file contents)
  // lives as long as the scanner: a new request sees the files as they are.
  clang::dependencies::DependencyScanningService Service;

  /// The path in the cache for \param ModuleName, if the file still declares
  /// that module. A path whose file changed or disappeared is dropped.
  std::optional<std::string>
  validatedSourceForModuleName(llvm::StringRef ModuleName,
                               const ProjectModules::CommandMangler &Mangler);
};

std::optional<ModuleDependencyScanner::ModuleDependencyInfo>
ModuleDependencyScanner::scan(PathRef FilePath,
                              const ProjectModules::CommandMangler &Mangler) {
  std::vector<tooling::CompileCommand> Candidates;
  {
    std::lock_guard<std::mutex> Lock(Cache->Mu);
    if (auto It = Cache->ExtraCommands.find(FilePath);
        It != Cache->ExtraCommands.end())
      Candidates.push_back(It->second);
  }
  if (Candidates.empty())
    Candidates = CDB->getCompileCommands(FilePath);
  if (Candidates.empty())
    return std::nullopt;

  // Choose the first candidates as the compile commands as the file.
  // Following the same logic with
  // DirectoryBasedGlobalCompilationDatabase::getCompileCommand.
  tooling::CompileCommand Cmd = std::move(Candidates.front());

  if (Mangler)
    Mangler(Cmd, FilePath);

  using namespace clang::tooling;

  // The result of an earlier scan stands while neither the file nor the command
  // changed. Files that cannot be examined (relative paths of a test database)
  // are scanned every time.
  ModuleScanCache::FileScan Key;
  Key.CommandHash = llvm::hash_combine_range(Cmd.CommandLine.begin(),
                                             Cmd.CommandLine.end());
  Key.CommandHash = llvm::hash_combine(Key.CommandHash, Cmd.Directory);
  bool CanCache = false;
  if (llvm::sys::path::is_absolute(FilePath))
    if (auto Status = TFS.view(std::nullopt)->status(FilePath)) {
      Key.ModificationTime = Status->getLastModificationTime();
      Key.Size = Status->getSize();
      CanCache = true;
    }
  auto ToInfo = [](const ModuleScanCache::FileScan &Scan)
      -> std::optional<ModuleDependencyInfo> {
    if (!Scan.Succeeded)
      return std::nullopt;
    return ModuleDependencyInfo{Scan.ModuleName, Scan.RequiredModules};
  };
  if (CanCache) {
    std::lock_guard<std::mutex> Lock(Cache->Mu);
    auto It = Cache->Files.find(FilePath);
    if (It != Cache->Files.end() &&
        It->second.ModificationTime == Key.ModificationTime &&
        It->second.Size == Key.Size && It->second.CommandHash == Key.CommandHash)
      return ToInfo(It->second);
  }

  llvm::SmallString<128> FilePathDir(FilePath);
  llvm::sys::path::remove_filename(FilePathDir);
  DependencyScanningTool ScanningTool(Service, TFS.view(FilePathDir));

  std::string S;
  llvm::raw_string_ostream OS(S);
  DiagnosticOptions DiagOpts;
  DiagOpts.ShowCarets = false;
  TextDiagnosticPrinter DiagConsumer(OS, DiagOpts);

  std::optional<P1689Rule> ScanningResult =
      ScanningTool.getP1689ModuleDependencyFile(Cmd, Cmd.Directory,
                                                DiagConsumer);

  ModuleScanCache::FileScan Scan = Key;
  if (!ScanningResult) {
    elog("Scanning modules dependencies for {0} failed: {1}", FilePath, S);
  } else {
    Scan.Succeeded = true;
    if (ScanningResult->Provides)
      Scan.ModuleName = ScanningResult->Provides->ModuleName;
    for (auto &Required : ScanningResult->Requires)
      Scan.RequiredModules.push_back(Required.ModuleName);
  }

  {
    std::lock_guard<std::mutex> Lock(Cache->Mu);
    // A file that stopped declaring the module it used to declare no longer
    // provides it.
    if (CanCache) {
      auto It = Cache->Files.find(FilePath);
      if (It != Cache->Files.end() && It->second.ModuleName &&
          It->second.ModuleName != Scan.ModuleName) {
        auto Old = Cache->ModuleNameToSource.find(*It->second.ModuleName);
        if (Old != Cache->ModuleNameToSource.end() && Old->second == FilePath)
          Cache->ModuleNameToSource.erase(Old);
      }
      Cache->Files[FilePath] = Scan;
    }
    if (Scan.ModuleName) {
      auto [Iter, Inserted] =
          Cache->ModuleNameToSource.try_emplace(*Scan.ModuleName, FilePath);

      if (!Inserted && Iter->second != FilePath) {
        elog("Detected multiple source files ({0}, {1}) declaring the same "
             "module: '{2}'. "
             "Now clangd may find the wrong source in such case.",
             Iter->second, FilePath, *Scan.ModuleName);
      }
    }
  }
  return ToInfo(Scan);
}

namespace {
// Directories that hold build products or third-party code rather than the
// project's own sources.
bool isIgnoredDirectory(llvm::StringRef Name) {
  return Name.starts_with(".") || Name.starts_with("build") ||
         Name.starts_with("cmake-build") || Name == "CMakeFiles" ||
         Name == "node_modules" || Name == "vcpkg_installed" || Name == "_deps" ||
         Name == "out";
}

bool hasModuleUnitExtension(llvm::StringRef Path) {
  llvm::StringRef Ext = llvm::sys::path::extension(Path);
  return Ext == ".ixx" || Ext == ".cppm" || Ext == ".cxxm" || Ext == ".c++m";
}

bool hasSourceExtension(llvm::StringRef Path) {
  llvm::StringRef Ext = llvm::sys::path::extension(Path);
  return Ext == ".cpp" || Ext == ".cc" || Ext == ".cxx" || Ext == ".c++";
}
} // namespace

void ModuleDependencyScanner::discoverUnlistedModules(
    const ProjectModules::CommandMangler &Mangler) {
  // The search is not repeated for every lookup of a module that does not
  // exist; what it found stays in the cache.
  constexpr auto MinInterval = std::chrono::seconds(1);
  {
    std::lock_guard<std::mutex> Lock(Cache->Mu);
    auto Now = std::chrono::steady_clock::now();
    if (Cache->LastDiscovery && Now - *Cache->LastDiscovery < MinInterval)
      return;
    Cache->LastDiscovery = Now;
  }

  constexpr unsigned MaxDepth = 8;
  constexpr unsigned MaxDirectories = 4000;
  constexpr unsigned MaxContentProbes = 2000;

  std::set<std::string> Listed;
  std::set<std::string> Roots;
  for (const std::string &File : CDB->getAllFiles()) {
    llvm::SmallString<256> Abs(File);
    if (!llvm::sys::path::is_absolute(Abs)) {
      // The database lists the file relative to the directory of its command.
      auto Cmds = CDB->getCompileCommands(File);
      if (Cmds.empty())
        continue;
      llvm::sys::path::make_absolute(Cmds.front().Directory, Abs);
    }
    llvm::sys::path::remove_dots(Abs, /*remove_dot_dot=*/true);
    Listed.insert(std::string(Abs));
    llvm::StringRef Dir = llvm::sys::path::parent_path(Abs);
    if (!Dir.empty())
      Roots.insert(Dir.str());
  }
  // Keep the outermost directories only: they are walked recursively.
  std::vector<std::string> Outer;
  for (const std::string &Root : Roots) {
    bool Covered = false;
    for (llvm::StringRef Parent = llvm::sys::path::parent_path(Root);
         !Parent.empty(); Parent = llvm::sys::path::parent_path(Parent))
      if (Roots.count(Parent.str())) {
        Covered = true;
        break;
      }
    if (!Covered)
      Outer.push_back(Root);
  }

  auto FS = TFS.view(std::nullopt);
  // The unit of a module declaration in a file that does not have one of the
  // extensions of module units: "export module X;" or "module X;".
  llvm::Regex ModuleDeclaration(
      "^[ \\t]*(export[ \\t]+)?module[ \\t]+[A-Za-z_][A-Za-z0-9_.:]*[ \\t]*;",
      llvm::Regex::Newline);

  std::vector<std::string> Found;
  unsigned Directories = 0, Probes = 0;
  auto Walk = [&](auto &&Self, llvm::StringRef Dir, unsigned Depth) -> void {
    if (Depth > MaxDepth || ++Directories > MaxDirectories)
      return;
    std::error_code EC;
    for (llvm::vfs::directory_iterator It = FS->dir_begin(Dir, EC), End;
         !EC && It != End; It.increment(EC)) {
      llvm::StringRef Path = It->path();
      llvm::StringRef Name = llvm::sys::path::filename(Path);
      if (It->type() == llvm::sys::fs::file_type::directory_file) {
        if (!isIgnoredDirectory(Name))
          Self(Self, Path, Depth + 1);
        continue;
      }
      if (It->type() != llvm::sys::fs::file_type::regular_file)
        continue;
      if (Listed.count(Path.str()))
        continue;
      if (hasModuleUnitExtension(Path)) {
        Found.push_back(Path.str());
      } else if (hasSourceExtension(Path) && Probes < MaxContentProbes) {
        ++Probes;
        auto Buf = FS->getBufferForFile(Path, /*FileSize=*/64 * 1024);
        if (Buf && ModuleDeclaration.match((*Buf)->getBuffer()))
          Found.push_back(Path.str());
      }
    }
  };
  for (const std::string &Root : Outer)
    Walk(Walk, Root, 0);

  std::sort(Found.begin(), Found.end());
  for (const std::string &File : Found)
    scan(File, Mangler);
}

void ModuleDependencyScanner::globalScan(
    const ProjectModules::CommandMangler &Mangler) {
  if (GlobalScanned)
    return;

  for (auto &File : CDB->getAllFiles())
    scan(File, Mangler);

  GlobalScanned = true;
}

PathRef ModuleDependencyScanner::getSourceForModuleName(
    llvm::StringRef ModuleName) const {
  assert(
      GlobalScanned &&
      "We should only call getSourceForModuleName after calling globalScan()");

  std::lock_guard<std::mutex> Lock(Cache->Mu);
  if (auto It = Cache->ModuleNameToSource.find(ModuleName);
      It != Cache->ModuleNameToSource.end())
    return It->second;

  return {};
}

std::optional<std::string> ModuleDependencyScanner::validatedSourceForModuleName(
    llvm::StringRef ModuleName, const ProjectModules::CommandMangler &Mangler) {
  std::string Source;
  {
    std::lock_guard<std::mutex> Lock(Cache->Mu);
    auto It = Cache->ModuleNameToSource.find(ModuleName);
    if (It == Cache->ModuleNameToSource.end())
      return std::nullopt;
    Source = It->second;
  }
  // The file may have changed or disappeared since it was scanned.
  auto Info = scan(Source, Mangler);
  if (Info && Info->ModuleName == ModuleName)
    return Source;
  std::lock_guard<std::mutex> Lock(Cache->Mu);
  auto It = Cache->ModuleNameToSource.find(ModuleName);
  if (It != Cache->ModuleNameToSource.end() && It->second == Source)
    Cache->ModuleNameToSource.erase(It);
  return std::nullopt;
}

std::string ModuleDependencyScanner::getSourceForModuleName(
    llvm::StringRef ModuleName, PathRef ImportingFile,
    const ProjectModules::CommandMangler &Mangler) {
  globalScan(Mangler);
  if (auto Source = validatedSourceForModuleName(ModuleName, Mangler))
    return *Source;

  // The file that declared the module is gone or declares something else now:
  // another file may declare it.
  if (GlobalScanned) {
    GlobalScanned = false;
    globalScan(Mangler);
    if (auto Source = validatedSourceForModuleName(ModuleName, Mangler))
      return *Source;
  }

  // A module unit that the database does not know about yet.
  discoverUnlistedModules(Mangler);
  if (auto Source = validatedSourceForModuleName(ModuleName, Mangler))
    return *Source;

  if (ModuleName != "std" && ModuleName != "std.compat")
    return {};
  auto Candidates = CDB->getCompileCommands(ImportingFile);
  if (Candidates.empty())
    return {};
  tooling::CompileCommand Importer = std::move(Candidates.front());
  if (Mangler)
    Mangler(Importer, ImportingFile);
  std::string Source;
  auto Cmd = resolveStandardLibraryModule(ModuleName, Importer, Source);
  if (!Cmd)
    return {};
  {
    std::lock_guard<std::mutex> Lock(Cache->Mu);
    Cache->ModuleNameToSource[ModuleName] = Source;
    Cache->ExtraCommands[Source] = std::move(*Cmd);
  }
  return Source;
}

std::optional<tooling::CompileCommand>
ModuleDependencyScanner::getCompileCommandForSource(PathRef File) const {
  std::lock_guard<std::mutex> Lock(Cache->Mu);
  if (auto It = Cache->ExtraCommands.find(File); It != Cache->ExtraCommands.end())
    return It->second;
  return std::nullopt;
}

std::vector<std::string> ModuleDependencyScanner::getRequiredModules(
    PathRef File, const ProjectModules::CommandMangler &Mangler) {
  auto ScanningResult = scan(File, Mangler);
  if (!ScanningResult)
    return {};

  return ScanningResult->RequiredModules;
}
} // namespace

/// TODO: The existing `ScanningAllProjectModules` is not efficient. See the
/// comments in ModuleDependencyScanner for detail.
///
/// In the future, we wish the build system can provide a well design
/// compilation database for modules then we can query that new compilation
/// database directly. Or we need to have a global long-live scanner to detect
/// the state of each file.
class ScanningAllProjectModules : public ProjectModules {
public:
  ScanningAllProjectModules(
      std::shared_ptr<const clang::tooling::CompilationDatabase> CDB,
      const ThreadsafeFS &TFS, std::shared_ptr<ModuleScanCache> Cache)
      : Scanner(CDB, TFS, std::move(Cache)) {}

  ~ScanningAllProjectModules() override = default;

  std::vector<std::string> getRequiredModules(PathRef File) override {
    return Scanner.getRequiredModules(File, Mangler);
  }

  void setCommandMangler(CommandMangler Mangler) override {
    this->Mangler = std::move(Mangler);
  }

  /// RequiredSourceFile is not used intentionally. See the comments of
  /// ModuleDependencyScanner for detail.
  std::string getSourceForModuleName(llvm::StringRef ModuleName,
                                     PathRef RequiredSourceFile) override {
    return Scanner.getSourceForModuleName(ModuleName, RequiredSourceFile,
                                          Mangler);
  }

  std::optional<tooling::CompileCommand>
  getCompileCommandForSource(PathRef File) override {
    return Scanner.getCompileCommandForSource(File);
  }

  std::string getModuleNameForSource(PathRef File) override {
    auto ScanningResult = Scanner.scan(File, Mangler);
    if (!ScanningResult || !ScanningResult->ModuleName)
      return {};

    return *ScanningResult->ModuleName;
  }

private:
  ModuleDependencyScanner Scanner;
  CommandMangler Mangler;
};

std::unique_ptr<ProjectModules> scanningProjectModules(
    std::shared_ptr<const clang::tooling::CompilationDatabase> CDB,
    const ThreadsafeFS &TFS, std::shared_ptr<ModuleScanCache> Cache) {
  if (!Cache)
    Cache = createModuleScanCache();
  return std::make_unique<ScanningAllProjectModules>(CDB, TFS, std::move(Cache));
}

} // namespace clang::clangd
