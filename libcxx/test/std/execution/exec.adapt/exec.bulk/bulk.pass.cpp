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

// namespace execution {
//   struct bulk_t { ... };
//   inline constexpr bulk_t bulk{};
//   struct bulk_chunked_t { ... };
//   inline constexpr bulk_chunked_t bulk_chunked{};
//   struct bulk_unchunked_t { ... };
//   inline constexpr bulk_unchunked_t bulk_unchunked{};
// }

#include <cassert>
#include <execution>
#include <type_traits>
#include <vector>

using namespace std::execution;

int main(int, char**) {
  // bulk_unchunked: f is invoked exactly once per index, in order, with the original args
  // (by lvalue reference, so mutations are visible afterward) forwarded through unchanged.
  {
    std::vector<int> seen;
    auto r = std::this_thread::sync_wait(
        just(10) | bulk_unchunked(seq, 4, [&](int __i, int& __v) {
          seen.push_back(__i);
          __v += __i;
        }));
    assert(r.has_value());
    assert(*r == std::tuple(10 + 0 + 1 + 2 + 3));
    assert(seen == (std::vector<int>{0, 1, 2, 3}));
  }
  // bulk_chunked: f is invoked with a [begin, end) range -- this fork's single-threaded
  // fallback invokes it exactly once, covering [0, shape) as a whole.
  {
    std::vector<std::pair<int, int>> chunks;
    auto r = std::this_thread::sync_wait(
        just(0) | bulk_chunked(seq, 5, [&](int __b, int __e, int& __v) {
          chunks.emplace_back(__b, __e);
          for (int __i = __b; __i < __e; ++__i)
            __v += __i;
        }));
    assert(r.has_value());
    assert(*r == std::tuple(0 + 1 + 2 + 3 + 4));
    assert(chunks == (std::vector<std::pair<int, int>>{{0, 5}}));
  }
  // bulk: expression-equivalent to bulk_chunked with an internally-looping f -- same
  // per-index visitation as bulk_unchunked, expressed through bulk_chunked's machinery.
  {
    std::vector<int> seen;
    auto r = std::this_thread::sync_wait(just(100) | bulk(seq, 3, [&](int __i, int& __v) {
                                            seen.push_back(__i);
                                            __v += __i;
                                          }));
    assert(r.has_value());
    assert(*r == std::tuple(100 + 0 + 1 + 2));
    assert(seen == (std::vector<int>{0, 1, 2}));
  }
  // Call-syntax and pipe-syntax equivalence.
  {
    auto r1 = std::this_thread::sync_wait(bulk_unchunked(just(0), seq, 2, [](int, int&) {}));
    auto r2 = std::this_thread::sync_wait(just(0) | bulk_unchunked(seq, 2, [](int, int&) {}));
    assert(r1.has_value() && r2.has_value());
  }
  // Completion signatures: bulk_unchunked's own set_value_t(int) shape passes through
  // unchanged (f's return value discarded, same args forwarded), with an added
  // set_error_t(exception_ptr) since a lambda invocation is never statically nothrow here.
  {
    auto sndr = just(0) | bulk_unchunked(seq, 2, [](int, int&) {});
    static_assert(
        std::is_same_v<completion_signatures_of_t<decltype(sndr), env<>>,
                        completion_signatures<set_value_t(int), set_error_t(std::exception_ptr)>>);
  }
  return 0;
}
