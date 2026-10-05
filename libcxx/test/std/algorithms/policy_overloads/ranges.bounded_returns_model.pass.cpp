//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: libcpp-has-no-incomplete-pstl

// P3179R9: the Returns of the execution-policy overloads of ranges::set_union, set_symmetric_difference,
// unique_copy and partial_sort_copy, checked against a literal transcription of the draft definitions
// ([alg.set.union], [alg.set.symmetric.difference], [alg.unique], [alg.partial.sort.copy]) for every pair of
// small sorted multisets and every output size.

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <execution>
#include <functional>
#include <vector>

using Seq = std::vector<int>;

struct Positions {
  std::ptrdiff_t first, second, out;
  bool operator==(const Positions&) const = default;
};

static std::ptrdiff_t count_of(const Seq& s, int v) { return std::ranges::count(s, v); }

// The elements that make up the output of a set operation: (value, range, index in that range), in sorted order.
struct Elem {
  int value;
  int range;
  std::ptrdiff_t index;
};

static std::vector<Elem> model_union(const Seq& a, const Seq& b) {
  std::vector<Elem> u;
  for (int v = 0; v < 8; ++v) {
    auto m = count_of(a, v), n = count_of(b, v);
    auto a0 = std::ranges::find(a, v) - a.begin();
    auto b0 = std::ranges::find(b, v) - b.begin();
    for (std::ptrdiff_t i = 0; i < m; ++i)
      u.push_back({v, 1, a0 + i});
    for (std::ptrdiff_t i = m; i < n; ++i) // the final max(n - m, 0) elements of the second range
      u.push_back({v, 2, b0 + i});
  }
  return u;
}

static std::vector<Elem> model_symmetric_difference(const Seq& a, const Seq& b) {
  std::vector<Elem> u;
  for (int v = 0; v < 8; ++v) {
    auto m = count_of(a, v), n = count_of(b, v);
    auto a0 = std::ranges::find(a, v) - a.begin();
    auto b0 = std::ranges::find(b, v) - b.begin();
    for (std::ptrdiff_t i = n; i < m; ++i) // the last m - n elements of the first range
      u.push_back({v, 1, a0 + i});
    for (std::ptrdiff_t i = m; i < n; ++i) // the last n - m elements of the second range
      u.push_back({v, 2, b0 + i});
  }
  return u;
}

static Positions model_set_union(const Seq& a, const Seq& b, std::ptrdiff_t out_size) {
  auto u = model_union(a, b);
  std::ptrdiff_t M = u.size(), N = std::min(M, out_size);
  if (N == M)
    return {(std::ptrdiff_t)a.size(), (std::ptrdiff_t)b.size(), N};
  std::ptrdiff_t A = 0, B = 0;
  for (int v = 0; v < 8; ++v) {
    auto n = count_of(b, v);
    std::ptrdiff_t k = 0, copied2 = 0;
    for (std::ptrdiff_t i = 0; i < N; ++i) {
      if (u[i].value != v)
        continue;
      if (u[i].range == 1)
        ++k;
      else
        ++copied2;
    }
    A += k;
    B += copied2 + std::min(k, n); // copied, plus the first min(k, n) elements of the second range are skipped
  }
  return {A, B, out_size};
}

static Positions model_set_symmetric_difference(const Seq& a, const Seq& b, std::ptrdiff_t out_size) {
  auto u = model_symmetric_difference(a, b);
  std::ptrdiff_t M = u.size(), N = std::min(M, out_size);
  if (N == M) // the draft says "N is equal to M + K" with K never defined; K = 0
    return {(std::ptrdiff_t)a.size(), (std::ptrdiff_t)b.size(), N};
  Elem e = u[N];
  auto copied = [&](int range, std::ptrdiff_t index) {
    for (std::ptrdiff_t i = 0; i < N; ++i)
      if (u[i].range == range && u[i].index == index)
        return true;
    return false;
  };
  std::ptrdiff_t A = 0, B = 0;
  auto visit = [&](const Seq& s, int range, std::ptrdiff_t& counter) {
    for (std::ptrdiff_t i = 0; i < (std::ptrdiff_t)s.size(); ++i) {
      bool is_copied = copied(range, i);
      // A non-copied element is skipped if it compares less than or equivalent to the (N + 1)th element, unless it is
      // from the same range as that element and does not precede it.
      bool skipped = !is_copied && s[i] <= e.value && !(range == e.range && i >= e.index);
      if (is_copied || skipped)
        ++counter;
    }
  };
  visit(a, 1, A);
  visit(b, 2, B);
  return {A, B, out_size};
}

static std::vector<Seq> all_sorted(int values, int max_multiplicity) {
  std::vector<Seq> result{{}};
  for (int v = 0; v < values; ++v) {
    std::vector<Seq> next;
    for (const Seq& s : result)
      for (int c = 0; c <= max_multiplicity; ++c) {
        Seq t = s;
        t.insert(t.end(), c, v);
        next.push_back(t);
      }
    result = std::move(next);
  }
  return result;
}

template <class Policy>
void test_set_operations(Policy&& policy) {
  auto seqs = all_sorted(4, 2);
  for (const Seq& a : seqs)
    for (const Seq& b : seqs)
      for (std::ptrdiff_t out_size = 0; out_size <= (std::ptrdiff_t)(a.size() + b.size()) + 1; ++out_size) {
        Seq out(out_size);
        {
          auto r = std::ranges::set_union(policy, a, b, out);
          Positions got{r.in1 - a.begin(), r.in2 - b.begin(), r.out - out.begin()};
          assert(got == model_set_union(a, b, out_size));
        }
        {
          auto r = std::ranges::set_symmetric_difference(policy, a, b, out);
          Positions got{r.in1 - a.begin(), r.in2 - b.begin(), r.out - out.begin()};
          assert(got == model_set_symmetric_difference(a, b, out_size));
        }
      }
}

// [alg.unique]: E(i) = comp(proj(*(i - 1)), proj(*i)); M = number of i with E(i) false; N = min(M, out size);
// returns {last, result + N} if N == M, otherwise {j, result_last} where j is the iterator with E(j) false and
// exactly N such iterators in [first, j).
template <class Policy>
void test_unique_copy(Policy&& policy) {
  auto successor = [](int prev, int cur) { return cur == prev + 1; }; // not symmetric: pins the argument order
  for (int len = 0; len <= 6; ++len) {
    int combos = 1;
    for (int i = 0; i < len; ++i)
      combos *= 3;
    for (int code = 0; code < combos; ++code) {
      Seq in;
      for (int c = code, i = 0; i < len; ++i, c /= 3)
        in.push_back(c % 3);
      for (int use_successor = 0; use_successor < 2; ++use_successor) {
        auto E = [&](std::size_t i) {
          if (i == 0)
            return false;
          return use_successor ? successor(in[i - 1], in[i]) : in[i - 1] == in[i];
        };
        std::ptrdiff_t M = 0;
        for (std::size_t i = 0; i < in.size(); ++i)
          M += !E(i);
        for (std::ptrdiff_t out_size = 0; out_size <= M + 1; ++out_size) {
          Seq out(out_size, -1);
          std::ptrdiff_t N = std::min(M, out_size);
          std::ptrdiff_t first_pos;
          if (N == M) {
            first_pos = in.size();
          } else {
            first_pos = -1;
            std::ptrdiff_t seen = 0;
            for (std::size_t j = 0; j < in.size(); ++j)
              if (!E(j)) {
                if (seen == N) {
                  first_pos = j;
                  break;
                }
                ++seen;
              }
            assert(first_pos >= 0);
          }
          std::ranges::unique_copy_result<Seq::iterator, Seq::iterator> r =
              use_successor ? std::ranges::unique_copy(policy, in, out, successor)
                            : std::ranges::unique_copy(policy, in, out);
          assert(r.in - in.begin() == first_pos);
          assert(r.out - out.begin() == (N == M ? N : out_size));
          Seq expected;
          for (std::size_t i = 0; i < in.size() && (std::ptrdiff_t)expected.size() < N; ++i)
            if (!E(i))
              expected.push_back(in[i]);
          for (std::ptrdiff_t i = 0; i < N; ++i)
            assert(out[i] == expected[i]);
        }
      }
    }
  }
}

struct Wrapped {
  int v;
  Wrapped() = default;
  Wrapped(int x) : v(x) {}
};

// [alg.partial.sort.copy]: the N smallest elements, sorted, and {last, result_first + N}; the projection of the
// input range is only applied to input elements and the one of the output range only to output elements.
template <class Policy>
void test_partial_sort_copy(Policy&& policy) {
  Seq base{4, 1, 3, 0, 2};
  std::ranges::sort(base);
  do {
    for (std::size_t out_size = 0; out_size <= base.size() + 1; ++out_size) {
      std::vector<Wrapped> out(out_size);
      auto r = std::ranges::partial_sort_copy(policy, base, out, std::ranges::less{}, std::identity{}, &Wrapped::v);
      std::size_t N = std::min(base.size(), out_size);
      assert(r.in == base.end());
      assert(r.out == out.begin() + N);
      for (std::size_t i = 0; i < N; ++i)
        assert(out[i].v == (int)i);

      std::vector<int> same(out_size);
      auto r2 = std::ranges::partial_sort_copy(policy, base, same, std::greater{});
      assert(r2.in == base.end() && r2.out == same.begin() + N);
      for (std::size_t i = 0; i < N; ++i)
        assert(same[i] == (int)(base.size() - 1 - i));
    }
  } while (std::ranges::next_permutation(base).found);
}

int main(int, char**) {
  test_set_operations(std::execution::seq);
  test_set_operations(std::execution::par);
  test_unique_copy(std::execution::seq);
  test_unique_copy(std::execution::par_unseq);
  test_partial_sort_copy(std::execution::seq);
  test_partial_sort_copy(std::execution::par);
  return 0;
}
