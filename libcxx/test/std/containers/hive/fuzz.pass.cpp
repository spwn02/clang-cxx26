//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <hive>

// Randomized differential test against std::list: interleave random
// insert/erase, mirroring each operation into a std::list, and periodically
// check that (a) the two containers hold the same multiset of elements and
// (b) every live hive iterator still dereferences to the value it was
// handed at insertion time (pointer/iterator stability across unrelated
// insertions and erasures). This is the test most likely to catch a
// skipfield bookkeeping bug -- hand-written cases tend to only exercise the
// patterns the author already thought of.
//
// Iteration count is deliberately modest (this runs as part of the normal
// test suite, not as a fuzzer): it's enough to exercise many block
// allocations, block retirements, and skipblock merges/splits with a small
// block size, without making the test suite slow.

#include <hive>
#include <list>
#include <vector>
#include <algorithm>
#include <random>
#include <cassert>
#include <utility>

int main(int, char**) {
  std::mt19937 rng(1);
  std::hive<int> h(std::hive_limits(4, 8)); // small blocks: more block churn per iteration
  std::list<int> ref;
  std::vector<std::pair<std::hive<int>::iterator, std::list<int>::iterator>> live;
  int next_tag = 0;

  auto verify = [&] {
    std::vector<int> hv(h.begin(), h.end());
    std::vector<int> lv(ref.begin(), ref.end());
    std::sort(hv.begin(), hv.end());
    std::sort(lv.begin(), lv.end());
    assert(hv == lv);
    assert(h.size() == ref.size());
    for (auto& [hit, lit] : live)
      assert(*hit == *lit);
  };

  const int kIterations = 20000;
  for (int iter = 0; iter < kIterations; ++iter) {
    bool do_insert = live.empty() || (rng() % 100 < 60 && live.size() < 300);
    if (do_insert) {
      int tag = next_tag++;
      auto hit = h.insert(tag);
      ref.push_back(tag);
      live.emplace_back(hit, std::prev(ref.end()));
    } else {
      size_t idx = rng() % live.size();
      auto [hit, lit] = live[idx];
      h.erase(hit);
      ref.erase(lit);
      live[idx] = live.back();
      live.pop_back();
    }
    if (iter % 200 == 0)
      verify();
  }
  verify();

  // Forward/backward traversal must agree after the randomized run.
  std::vector<int> fwd(h.begin(), h.end());
  std::vector<int> bwd;
  for (auto it = h.end(); it != h.begin();) {
    --it;
    bwd.push_back(*it);
  }
  std::reverse(bwd.begin(), bwd.end());
  assert(fwd == bwd);

  // Drain everything and confirm a clean empty state.
  while (!live.empty()) {
    auto [hit, lit] = live.back();
    live.pop_back();
    h.erase(hit);
    ref.erase(lit);
  }
  assert(h.empty() && h.size() == 0 && h.begin() == h.end());

  return 0;
}
