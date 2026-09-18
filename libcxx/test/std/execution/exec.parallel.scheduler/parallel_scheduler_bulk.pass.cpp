//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: no-threads
// UNSUPPORTED: libcpp-has-no-incomplete-pstl

// <execution>
//
// Pass 2/3 (P2079R10, see docs/design/parallel_scheduler_p2079.md): bulk_chunked_t's and
// bulk_unchunked_t's parallel_scheduler completion-scheduler probe/dispatch. Both share the
// same worker-pool job machinery (see <__execution/bulk.h>'s __bulk_parallel_job); they differ
// only in whether each worker invokes f once for its whole sub-range (chunked) or once per
// index within it (unchunked).

#include <atomic>
#include <cassert>
#include <execution>
#include <mutex>
#include <set>
#include <thread>
#include <vector>

using namespace std::execution;

int main(int, char**) {
  // Correctness: every index in [0, shape) is covered by exactly one chunk, exactly once --
  // shape deliberately not divisible by any small chunk count, to catch an off-by-one in the
  // chunk-boundary arithmetic. par is otherwise-unused here (this fork's bulk_chunked doesn't
  // branch on Policy beyond requiring is_execution_policy_v), matching the existing
  // exec.bulk/bulk.pass.cpp test's own use of `seq`.
  {
    constexpr int kShape = 997;
    std::vector<std::atomic<int>> hits(kShape);
    for (auto& h : hits) {
      h.store(0);
    }

    auto r = std::this_thread::sync_wait(schedule(get_parallel_scheduler()) |
                                          bulk_chunked(par, kShape, [&](int b, int e) {
                                            for (int i = b; i < e; ++i) {
                                              hits[i].fetch_add(1, std::memory_order_relaxed);
                                            }
                                          }));
    assert(r.has_value());
    for (int i = 0; i < kShape; ++i) {
      assert(hits[i].load() == 1);
    }
  }

  // The customization is actually observable: chunks run on more than one distinct thread id
  // when the pool has more than one worker (true in practice on this test's build host) --
  // asserting real parallelism, not merely correctness that a sequential fallback would also
  // satisfy.
  if (std::thread::hardware_concurrency() > 1) {
    constexpr int kShape = 200000;
    std::mutex mtx;
    std::set<std::thread::id> ids;

    auto r = std::this_thread::sync_wait(schedule(get_parallel_scheduler()) |
                                          bulk_chunked(par, kShape, [&](int, int) {
                                            std::lock_guard<std::mutex> lock(mtx);
                                            ids.insert(std::this_thread::get_id());
                                          }));
    assert(r.has_value());
    assert(ids.size() > 1);
  }

  // bulk() itself (not just bulk_chunked()) becomes parallel for free, since bulk_t composes
  // over bulk_chunked_t -- exercising the exact chunk-boundary loop bulk_t's new_f wraps
  // around each chunk's [begin, end) range.
  {
    constexpr int kShape = 997;
    std::vector<std::atomic<int>> hits(kShape);
    for (auto& h : hits) {
      h.store(0);
    }

    auto r = std::this_thread::sync_wait(
        schedule(get_parallel_scheduler()) | bulk(par, kShape, [&](int i) { hits[i].fetch_add(1); }));
    assert(r.has_value());
    for (int i = 0; i < kShape; ++i) {
      assert(hits[i].load() == 1);
    }
  }

  // TRY-EVAL semantics: a throwing chunk still completes the whole operation via set_error,
  // not std::terminate (unlike the parallel *range algorithms*' [algorithms.parallel.exceptions]
  // rule -- [exec.bulk] uses ordinary TRY-EVAL).
  {
    constexpr int kShape = 64;
    bool caught          = false;
    try {
      std::this_thread::sync_wait(
          schedule(get_parallel_scheduler()) | bulk_chunked(par, kShape, [](int, int) { throw 42; }));
    } catch (int v) {
      caught = true;
      assert(v == 42);
    }
    assert(caught);
  }

  // Non-parallel_scheduler completion schedulers are unaffected: bulk_chunked still falls back
  // to its existing single-threaded behavior (verified against the same shape the exec.bulk
  // suite's own test already covers, just re-confirming no regression from the new probe).
  {
    std::vector<std::pair<int, int>> chunks;
    auto r = std::this_thread::sync_wait(just(0) | bulk_chunked(seq, 5, [&](int b, int e, int& v) {
                                            chunks.emplace_back(b, e);
                                            v += (e - b);
                                          }));
    assert(r.has_value());
    assert(*r == std::tuple(5));
    assert(chunks.size() == 1);
    assert((chunks[0] == std::pair(0, 5)));
  }

  // bulk_unchunked_t: same parallel_scheduler probe as bulk_chunked_t (Pass 3), but each worker
  // invokes f once per index in its own sub-range rather than once for the whole sub-range --
  // correctness: every index in [0, shape) is invoked exactly once, shape again deliberately not
  // divisible by any small chunk count.
  {
    constexpr int kShape = 997;
    std::vector<std::atomic<int>> hits(kShape);
    for (auto& h : hits) {
      h.store(0);
    }

    auto r = std::this_thread::sync_wait(
        schedule(get_parallel_scheduler()) | bulk_unchunked(par, kShape, [&](int i) {
          hits[i].fetch_add(1, std::memory_order_relaxed);
        }));
    assert(r.has_value());
    for (int i = 0; i < kShape; ++i) {
      assert(hits[i].load() == 1);
    }
  }

  // The customization is actually observable for bulk_unchunked_t too: indices run on more than
  // one distinct thread id when the pool has more than one worker.
  if (std::thread::hardware_concurrency() > 1) {
    constexpr int kShape = 200000;
    std::mutex mtx;
    std::set<std::thread::id> ids;

    auto r = std::this_thread::sync_wait(
        schedule(get_parallel_scheduler()) | bulk_unchunked(par, kShape, [&](int) {
          std::lock_guard<std::mutex> lock(mtx);
          ids.insert(std::this_thread::get_id());
        }));
    assert(r.has_value());
    assert(ids.size() > 1);
  }

  // TRY-EVAL semantics apply to bulk_unchunked_t's parallel dispatch identically to
  // bulk_chunked_t's: a throwing index still completes the whole operation via set_error.
  {
    constexpr int kShape = 64;
    bool caught          = false;
    try {
      std::this_thread::sync_wait(
          schedule(get_parallel_scheduler()) | bulk_unchunked(par, kShape, [](int) { throw 42; }));
    } catch (int v) {
      caught = true;
      assert(v == 42);
    }
    assert(caught);
  }

  // Non-parallel_scheduler completion schedulers are unaffected for bulk_unchunked_t either:
  // still falls back to invoking f once per index, in order, on the calling thread.
  {
    std::vector<int> indices;
    auto r = std::this_thread::sync_wait(just(0) | bulk_unchunked(seq, 5, [&](int i, int& v) {
                                            indices.push_back(i);
                                            v += 1;
                                          }));
    assert(r.has_value());
    assert(*r == std::tuple(5));
    assert((indices == std::vector<int>{0, 1, 2, 3, 4}));
  }

  return 0;
}
