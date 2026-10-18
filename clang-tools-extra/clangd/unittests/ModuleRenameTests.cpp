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
    if (!S.Name.empty())
      Result.push_back((S.IsModuleDeclaration ? "module " : "import ") + S.Name);
  return Result;
}

// "module A:P", "import :Q": the partitions that the declarations spell.
std::vector<std::string> partitions(llvm::StringRef Code) {
  std::vector<std::string> Result;
  for (const ModuleNameSpelling &S : moduleNameSpellings(Code))
    if (!S.Partition.empty())
      Result.push_back((S.IsModuleDeclaration ? "module " : "import ") +
                       S.Name + ":" + S.Partition);
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
  EXPECT_EQ(Spellings[0].Partition, "P.Q");
  EXPECT_NE(Spellings[0].PartitionBegin, 0u);
  EXPECT_EQ(Spellings[1].Name, "A.B");
  // Attributes may follow.
  EXPECT_THAT(names("export module A [[attr]];\nimport B [[attr]];\n"),
              ElementsAre("module A", "import B"));
}

TEST(ModuleRenameTest, Partitions) {
  EXPECT_THAT(partitions("export module A.B:P.Q;\nmodule C:R;\n"
                         "export import :S;\nimport :T.U;\nimport D;\n"),
              ElementsAre("module A.B:P.Q", "module C:R", "import :S",
                          "import :T.U"));
  // The private module fragment is not a partition, and nor are header units.
  EXPECT_THAT(partitions("module A;\nmodule :private;\nimport <v>;\n"),
              IsEmpty());
  EXPECT_EQ(moduleOfUnit("module;\n#include <x>\nexport module A.B:P;\n"),
            "A.B");
  EXPECT_EQ(moduleOfUnit("import :P;\nint x;\n"), std::nullopt);
}

TEST(ModuleRenameTest, WhatIsNotAName) {
  // module; module :private; header units: not names.
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
  EXPECT_TRUE(OnPartition->isInPartition(29));
  EXPECT_EQ(OnPartition->Partition, "Access");
  // The colon belongs to the name in front of it; what follows it, to the
  // partition.
  EXPECT_FALSE(moduleNameSpellingAt(Code, 26)->isInPartition(26));
  EXPECT_TRUE(moduleNameSpellingAt(Code, 27)->isInPartition(27));
  // `import :P;` has a partition and no name.
  const std::string Imp = "module M;\nimport :Part.Deep;\n";
  auto OnImport = moduleNameSpellingAt(Imp, 17);
  ASSERT_TRUE(OnImport);
  EXPECT_EQ(OnImport->Name, "");
  EXPECT_EQ(OnImport->Partition, "Part.Deep");
  EXPECT_TRUE(OnImport->isInPartition(17));
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

TEST(ModuleRenameTest, ValidPartitionNames) {
  EXPECT_FALSE(bool(validatePartitionName("Part")));
  EXPECT_FALSE(bool(validatePartitionName("Part.Deep")));
  // The rule about `std` is for the names of modules ([module.unit]).
  EXPECT_FALSE(bool(validatePartitionName("std")));
  EXPECT_FALSE(bool(validatePartitionName("std23")));
  for (llvm::StringRef Bad : {"", "a b", "a:b", "a..b", "1a", "module",
                              "a.import", "__x", "_X", "a__b"}) {
    llvm::Error Err = validatePartitionName(Bad);
    EXPECT_TRUE(bool(Err)) << Bad;
    llvm::consumeError(std::move(Err));
  }
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
  auto Edits = renameModuleInFiles("Miracle", "Hello", {A, B, C, U}, FS);
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

  auto Meta = renameModuleInFiles("Miracle.Meta", "Hello.World", {A, B, C, U}, FS);
  ASSERT_TRUE(bool(Meta)) << llvm::toString(Meta.takeError());
  EXPECT_EQ(Meta->size(), 3u);
  EXPECT_FALSE(Meta->count(A));
  EXPECT_EQ(applied((*Meta)[C]), "export module Hello.World;\n");
}

TEST_F(ModuleRenameFilesTest, MissingFilesAndManyFiles) {
  std::string A = write("A.cppm", "export module M;\n");
  std::string B = write("B.cpp", "import M;\n");
  std::vector<std::string> Files = {A, B, Dir.str().str() + "/gone.cpp"};
  // A rename is not limited by a number of files (the limit of the rename of
  // a symbol is for the cost of parsing them): all of them change, or none.
  for (int I = 0; I < 120; ++I)
    Files.push_back(write("U" + std::to_string(I) + ".cpp", "import M;\n"));
  auto Edits = renameModuleInFiles("M", "N", Files, FS);
  ASSERT_TRUE(bool(Edits)) << llvm::toString(Edits.takeError());
  EXPECT_EQ(Edits->size(), 122u);
}

TEST_F(ModuleRenameFilesTest, FilesThatCanImport) {
  auto In = [&](llvm::StringRef Sub, llvm::StringRef Name) {
    llvm::SmallString<256> D(Dir);
    llvm::sys::path::append(D, Sub);
    EXPECT_FALSE(llvm::sys::fs::create_directories(D));
    return write((Sub + "/" + Name).str(), "import M;\n");
  };
  std::string Src = In("src", "a.cpp");
  std::string Consumer = In("tests/consumer", "main.cxx"); // not in a database
  std::string Header = In("include/deep/er", "all.hpp");
  std::string Unit = In("src", "m.ixx");
  std::string Build = In("build/tests", "gen.cpp");
  std::string Hidden = In(".cache", "x.cpp");
  std::string Other = In("src", "notes.txt");
  auto Found = findFilesThatCanImport({Dir.str().str()}, FS);
  ASSERT_TRUE(bool(Found)) << llvm::toString(Found.takeError());
  EXPECT_THAT(*Found, testing::UnorderedElementsAre(Src, Consumer, Header, Unit));
  // A search that is cut off is an error, not a part of the files.
  auto Cut = findFilesThatCanImport({Dir.str().str()}, FS, 2);
  ASSERT_FALSE(bool(Cut));
  EXPECT_THAT(llvm::toString(Cut.takeError()),
              testing::HasSubstr("nothing is renamed"));
  (void)Build; (void)Hidden; (void)Other;
}

TEST_F(ModuleRenameFilesTest, ModuleAlreadyDeclared) {
  std::string A = write("A.cppm", "export module A;\n");
  std::string B = write("B.cppm", "export module B:Part;\n");
  std::string U = write("U.cpp", "import A;\n");
  // B has only a partition declared: the name is taken all the same.
  auto Edits = renameModuleInFiles("A", "B", {A, B, U}, FS);
  ASSERT_FALSE(bool(Edits));
  EXPECT_THAT(llvm::toString(Edits.takeError()),
              testing::HasSubstr("already names a module"));
}

TEST_F(ModuleRenameFilesTest, Partition) {
  // A:P is renamed in its declaration and in the imports of the units of A;
  // B:P is another partition and C imports another module's P.
  std::string AI = write("AI.cppm", "export module A;\nexport import :P;\nimport :Q;\n");
  std::string AP = write("AP.cppm", "export module A:P;\nimport :Q;\n");
  std::string AQ = write("AQ.cppm", "module A:Q;\n");
  std::string AImpl = write("AImpl.cpp", "module A;\nimport :P;\n");
  std::string BP = write("BP.cppm", "export module B:P;\n");
  std::string BI = write("BI.cppm", "export module B;\nexport import :P;\n");
  std::string U = write("U.cpp", "import A;\nimport B;\n");
  auto Edits = renamePartitionInFiles("A", "P", "Part", {AI, AP, AQ, AImpl, BP, BI, U}, FS);
  ASSERT_TRUE(bool(Edits)) << llvm::toString(Edits.takeError());
  EXPECT_EQ(Edits->size(), 3u);
  EXPECT_EQ(applied((*Edits)[AI]),
            "export module A;\nexport import :Part;\nimport :Q;\n");
  EXPECT_EQ(applied((*Edits)[AP]), "export module A:Part;\nimport :Q;\n");
  EXPECT_EQ(applied((*Edits)[AImpl]), "module A;\nimport :Part;\n");
  EXPECT_FALSE(Edits->count(BP));
  EXPECT_FALSE(Edits->count(BI));
  // A dotted partition is renamed whole.
  std::string D = write("D.cppm", "export module D:a.b;\nimport :a.bc;\nimport :a.b;\n");
  auto Dotted = renamePartitionInFiles("D", "a.b", "x", {D}, FS);
  ASSERT_TRUE(bool(Dotted)) << llvm::toString(Dotted.takeError());
  EXPECT_EQ(applied((*Dotted)[D]),
            "export module D:x;\nimport :a.bc;\nimport :x;\n");
}

TEST_F(ModuleRenameFilesTest, PartitionRefusals) {
  std::string AI = write("AI.cppm", "export module A;\nexport import :P;\n");
  std::string AP = write("AP.cppm", "export module A:P;\n");
  std::string AQ = write("AQ.cppm", "module A:Q;\n");
  // The module has a partition of the new name already.
  auto Taken = renamePartitionInFiles("A", "P", "Q", {AI, AP, AQ}, FS);
  ASSERT_FALSE(bool(Taken));
  EXPECT_THAT(llvm::toString(Taken.takeError()),
              testing::HasSubstr("already has a partition 'Q'"));
  // Another module's partition of that name is no obstacle.
  std::string BQ = write("BQ.cppm", "export module B:Z;\n");
  auto Other = renamePartitionInFiles("A", "P", "Z", {AI, AP, BQ}, FS);
  ASSERT_TRUE(bool(Other)) << llvm::toString(Other.takeError());
  EXPECT_EQ(Other->size(), 2u);
}

TEST_F(ModuleRenameFilesTest, PartitionImportInAHeader) {
  std::string AP = write("AP.cppm", "export module A:P;\n");
  std::string Header = write("inc.hpp", "import :P;\n");
  auto Edits = renamePartitionInFiles("A", "P", "R", {AP, Header}, FS);
  ASSERT_TRUE(bool(Edits)) << llvm::toString(Edits.takeError());
  // No other module has a partition P: the header can only be for A.
  EXPECT_EQ(Edits->size(), 2u);
  EXPECT_EQ(applied((*Edits)[Header]), "import :R;\n");
  // With B:P too the header can be for either: refused.
  std::string BP = write("BP.cppm", "export module B:P;\n");
  auto Ambiguous = renamePartitionInFiles("A", "P", "R", {AP, Header, BP}, FS);
  ASSERT_FALSE(bool(Ambiguous));
  EXPECT_THAT(llvm::toString(Ambiguous.takeError()),
              testing::HasSubstr("can be for the module"));
}

} // namespace
} // namespace clangd
} // namespace clang
