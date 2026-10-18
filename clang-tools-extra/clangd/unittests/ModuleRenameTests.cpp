//===-- ModuleRenameTests.cpp - Rename of a module ------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ModuleRename.h"
#include "support/ThreadsafeFS.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace clang {
namespace clangd {
namespace {

using ::testing::ElementsAre;
using ::testing::IsEmpty;

std::vector<std::string> names(llvm::StringRef Code) {
  std::vector<std::string> Result;
  for (const ModuleNameSpelling &S : moduleNameSpellings(Code))
    Result.push_back((S.IsModuleDeclaration ? "module " : "import ") + S.Name);
  return Result;
}

TEST(ModuleRenameTest, SpellingsOfDeclarations) {
  EXPECT_THAT(names("export module Miracle.Meta;\n"
                    "import std;\n"
                    "export import Miracle;\n"
                    "import Miracle.Meta;\n"),
              ElementsAre("module Miracle.Meta", "import std",
                          "import Miracle", "import Miracle.Meta"));
  // The name is the whole dotted name, a partition is not part of it.
  auto Spellings = moduleNameSpellings("module A.B:P.Q;\nimport A.B:R;\n");
  ASSERT_EQ(Spellings.size(), 2u);
  EXPECT_EQ(Spellings[0].Name, "A.B");
  EXPECT_NE(Spellings[0].PartitionBegin, 0u);
  EXPECT_EQ(Spellings[1].Name, "A.B");
  // Attributes may follow.
  EXPECT_THAT(names("export module A [[attr]];\nimport B [[attr]];\n"),
              ElementsAre("module A", "import B"));
}

TEST(ModuleRenameTest, WhatIsNotAName) {
  // import :P; module; module :private; header units: not names.
  EXPECT_THAT(names("module;\n#include <x>\nexport module A:P;\n"
                    "import :Q;\nimport <vector>;\nimport \"h.h\";\n"
                    "module :private;\n"),
              ElementsAre("module A"));
  // Comments and literals.
  EXPECT_THAT(names("// import Comment;\n/* import Block; */\n"
                    "const char *S = \"import Str;\";\n"
                    "const char *R = R\"x(\nimport Raw;\n)x\";\n"),
              IsEmpty());
  // A declaration begins its line.
  EXPECT_THAT(names("int a; import Same;\nstruct S { int import; };\n"),
              IsEmpty());
}

TEST(ModuleRenameTest, SpellingAt) {
  const std::string Code = "export module Miracle.Meta:Access;\nimport Miracle;\n";
  // On any part of the dotted name: the whole name.
  for (unsigned Offset : {14u, 20u, 24u}) {
    auto S = moduleNameSpellingAt(Code, Offset);
    ASSERT_TRUE(S) << Offset;
    EXPECT_EQ(S->Name, "Miracle.Meta");
    EXPECT_EQ(Code.substr(S->Begin, S->End - S->Begin), "Miracle.Meta");
  }
  // After the name (the cursor right behind it) too, and in the partition.
  EXPECT_TRUE(moduleNameSpellingAt(Code, 26));
  auto OnPartition = moduleNameSpellingAt(Code, 29);
  ASSERT_TRUE(OnPartition);
  EXPECT_LE(OnPartition->PartitionBegin, 29u);
  // Elsewhere there is none.
  EXPECT_FALSE(moduleNameSpellingAt(Code, 3));
  EXPECT_FALSE(moduleNameSpellingAt(Code, Code.size()));
}

TEST(ModuleRenameTest, DeclarationSpellingsNeedTheSemicolon) {
  // The imports of a unit are required again when one becomes complete.
  EXPECT_NE(moduleDeclarationSpellings("export module A:P;\nimport std"),
            moduleDeclarationSpellings("export module A:P;\nimport std;"));
  EXPECT_EQ(moduleDeclarationSpellings("export module A;\nimport B;\nint x;"),
            moduleDeclarationSpellings("export module A;\nimport B;\nint y;"));
  EXPECT_THAT(moduleDeclarationSpellings("export module A;\nexport import :P;\n"
                                         "import B:Q;\nimport C.D;\n"),
              ElementsAre("module A ;", "import :P ;", "import B:Q ;",
                          "import C.D ;"));
}

TEST(ModuleRenameTest, ValidNames) {
  EXPECT_FALSE(bool(validateModuleName("Miracle")));
  EXPECT_FALSE(bool(validateModuleName("Miracle.Meta")));
  for (llvm::StringRef Bad :
       {"", "a b", "a:b", "a..b", ".a", "a.", "1a", "module", "a.import",
        "std", "std.compat", "std23", "std23.x", "__x", "_X", "a__b", "a-b"}) {
    llvm::Error Err = validateModuleName(Bad);
    EXPECT_TRUE(bool(Err)) << Bad;
    llvm::consumeError(std::move(Err));
  }
  // std followed by something else is not reserved by this rule.
  EXPECT_FALSE(bool(validateModuleName("stdx")));
  EXPECT_FALSE(bool(validateModuleName("a.std")));
}

std::string applied(const Edit &E) {
  auto R = E.apply();
  EXPECT_TRUE(bool(R));
  if (!R) {
    llvm::consumeError(R.takeError());
    return {};
  }
  return *R;
}

class ModuleRenameFilesTest : public ::testing::Test {
protected:
  void SetUp() override {
    ASSERT_FALSE(llvm::sys::fs::createUniqueDirectory("module-rename", Dir));
  }
  void TearDown() override { llvm::sys::fs::remove_directories(Dir); }
  std::string write(llvm::StringRef Name, llvm::StringRef Text) {
    llvm::SmallString<256> Path(Dir);
    llvm::sys::path::append(Path, Name);
    std::error_code EC;
    llvm::raw_fd_ostream OS(Path, EC);
    EXPECT_FALSE(EC);
    OS << Text;
    return std::string(Path);
  }
  llvm::SmallString<128> Dir;
  RealThreadsafeFS FS;
};

TEST_F(ModuleRenameFilesTest, WholeNamesOnly) {
  // Miracle and Miracle.Meta are two modules.
  std::string A = write("A.cppm", "export module Miracle;\nexport import :Part;\n");
  std::string B = write("B.cppm", "export module Miracle:Part;\nimport Miracle.Meta;\n");
  std::string C = write("C.cppm", "export module Miracle.Meta;\n");
  std::string U = write("U.cpp", "import Miracle;\nimport Miracle.Meta;\n"
                                 "// import Miracle;\nconst char *s = \"import Miracle;\";\n");
  auto Edits = renameModuleInFiles("Miracle", "Hello", {A, B, C, U}, FS, 50);
  ASSERT_TRUE(bool(Edits)) << llvm::toString(Edits.takeError());
  // C names only Miracle.Meta: untouched.
  EXPECT_EQ(Edits->size(), 3u);
  EXPECT_FALSE(Edits->count(C));
  EXPECT_EQ(applied((*Edits)[A]),
            "export module Hello;\nexport import :Part;\n");
  // The primary name of a partition declaration changes, the partition not.
  EXPECT_EQ(applied((*Edits)[B]),
            "export module Hello:Part;\nimport Miracle.Meta;\n");
  EXPECT_EQ(applied((*Edits)[U]),
            "import Hello;\nimport Miracle.Meta;\n// import Miracle;\n"
            "const char *s = \"import Miracle;\";\n");

  auto Meta = renameModuleInFiles("Miracle.Meta", "Hello.World", {A, B, C, U}, FS, 50);
  ASSERT_TRUE(bool(Meta)) << llvm::toString(Meta.takeError());
  EXPECT_EQ(Meta->size(), 3u);
  EXPECT_FALSE(Meta->count(A));
  EXPECT_EQ(applied((*Meta)[C]), "export module Hello.World;\n");
}

TEST_F(ModuleRenameFilesTest, LimitAndMissingFiles) {
  std::string A = write("A.cppm", "export module M;\n");
  std::string B = write("B.cpp", "import M;\n");
  auto Refused = renameModuleInFiles("M", "N", {A, B, Dir.str().str() + "/gone.cpp"}, FS, 1);
  ASSERT_FALSE(bool(Refused));
  EXPECT_THAT(llvm::toString(Refused.takeError()),
              testing::HasSubstr("--rename-file-limit"));
  auto Fine = renameModuleInFiles("M", "N", {A, B, Dir.str().str() + "/gone.cpp"}, FS, 2);
  ASSERT_TRUE(bool(Fine));
  EXPECT_EQ(Fine->size(), 2u);
}

} // namespace
} // namespace clangd
} // namespace clang
