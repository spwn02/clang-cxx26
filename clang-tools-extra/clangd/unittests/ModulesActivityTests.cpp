//===-- ModulesActivityTests.cpp --------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ModulesActivity.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include <string>
#include <vector>

namespace clang {
namespace clangd {
namespace {

using Phase = ModulesActivity::Phase;

struct Recorder {
  struct Event {
    Phase P;
    unsigned Enqueued, Completed;
    std::string Current;
    bool Idle;
  };
  std::vector<Event> Events;
  ModulesActivity Activity;
  Recorder() {
    Activity.setListener([this](Phase P, const ModulesActivity::State &S) {
      Events.push_back({P, S.Enqueued, S.Completed, S.Current, S.idle()});
    });
  }
};

TEST(ModulesActivityTest, ItemsThatTakeNoTimeAreNotReported) {
  Recorder R;
  {
    ModulesActivity::Batch B(&R.Activity, Phase::Scanning, 3);
    B.done();
    B.done();
    B.done();
  }
  EXPECT_TRUE(R.Events.empty());
  // A batch that is dropped is completed.
  {
    ModulesActivity::Batch B(&R.Activity, Phase::Building, 2);
  }
  EXPECT_TRUE(R.Events.empty());
}

TEST(ModulesActivityTest, ItemsThatTakeTimeAreReportedUntilTheEnd) {
  Recorder R;
  {
    ModulesActivity::Batch B(&R.Activity, Phase::Building, 3);
    B.done(); // took no time
    B.begin("Core");
    B.done();
    B.begin("Util:Part");
    // The batch ends before the last item is done: it is completed.
  }
  ASSERT_EQ(R.Events.size(), 4u);
  EXPECT_EQ(R.Events[0].Current, "Core");
  EXPECT_EQ(R.Events[0].Completed, 1u);
  EXPECT_EQ(R.Events[0].Enqueued, 3u);
  EXPECT_FALSE(R.Events[0].Idle);
  EXPECT_EQ(R.Events[1].Completed, 2u);
  EXPECT_EQ(R.Events[2].Current, "Util:Part");
  EXPECT_FALSE(R.Events[2].Idle);
  EXPECT_TRUE(R.Events[3].Idle);
  EXPECT_EQ(R.Events[3].Completed, 3u);
  EXPECT_EQ(R.Events[3].P, Phase::Building);
}

TEST(ModulesActivityTest, ABarOfOnePhaseStartsOverWhenIdle) {
  Recorder R;
  for (int I = 0; I != 2; ++I) {
    ModulesActivity::Batch B(&R.Activity, Phase::Scanning, 1);
    B.begin("a.cppm");
    B.done();
  }
  ASSERT_EQ(R.Events.size(), 4u);
  // The second batch does not count the items of the first.
  EXPECT_EQ(R.Events[2].Enqueued, 1u);
  EXPECT_EQ(R.Events[2].Completed, 0u);
}

TEST(ModulesActivityTest, BatchesOfAPhaseAreCountedTogether) {
  Recorder R;
  ModulesActivity::Batch A(&R.Activity, Phase::Building, 1);
  {
    ModulesActivity::Batch B(&R.Activity, Phase::Building, 1);
    B.begin("b");
    B.done();
    // A is still busy: the phase is not idle.
    ASSERT_EQ(R.Events.size(), 2u);
    EXPECT_FALSE(R.Events.back().Idle);
    EXPECT_EQ(R.Events.back().Enqueued, 2u);
  }
  A.begin("a");
  A.done();
  EXPECT_TRUE(R.Events.back().Idle);
  // The phases are independent.
  EXPECT_EQ(R.Events.back().P, Phase::Building);
}

TEST(ModulesActivityTest, WithoutAnActivityNothingHappens) {
  ModulesActivity::Batch B(nullptr, Phase::Building, 2);
  B.begin("x");
  B.done();
}

} // namespace
} // namespace clangd
} // namespace clang
