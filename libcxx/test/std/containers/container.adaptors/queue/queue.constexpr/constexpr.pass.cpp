//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <queue>
//
// P3372R3: constexpr containers and adaptors -- queue's full member surface
// is constexpr. queue's Container requirements include pop_front(), which
// std::vector does not provide (only std::deque/std::list do, and neither
// is constexpr yet in this fork) -- so a minimal fixed-capacity ring buffer
// satisfying exactly the operations queue needs (push_back/pop_front/front/
// back/empty/size) stands in for the underlying container here.

#include <array>
#include <cstddef>
#include <queue>
#include <utility>

template <class T, std::size_t N>
struct FixedRing {
  using value_type      = T;
  using reference       = T&;
  using const_reference = const T&;
  using size_type       = std::size_t;

  std::array<T, N> buf_{};
  std::size_t begin_ = 0;
  std::size_t size_  = 0;

  constexpr bool empty() const { return size_ == 0; }
  constexpr std::size_t size() const { return size_; }
  constexpr T& front() { return buf_[begin_]; }
  constexpr const T& front() const { return buf_[begin_]; }
  constexpr T& back() { return buf_[(begin_ + size_ - 1) % N]; }
  constexpr const T& back() const { return buf_[(begin_ + size_ - 1) % N]; }
  constexpr void push_back(const T& v) {
    buf_[(begin_ + size_) % N] = v;
    ++size_;
  }
  template <class... Args>
  constexpr T& emplace_back(Args&&... args) {
    T& slot = buf_[(begin_ + size_) % N];
    slot    = T(std::forward<Args>(args)...);
    ++size_;
    return slot;
  }
  constexpr void pop_front() {
    begin_ = (begin_ + 1) % N;
    --size_;
  }

  // Value-equality/ordering over the logical (in-order) elements only -- not
  // a bitwise/member-wise comparison, since two rings holding the same
  // logical sequence can have different `begin_` offsets or stale trailing
  // slots in `buf_`.
  constexpr bool operator==(const FixedRing& o) const {
    if (size_ != o.size_)
      return false;
    for (std::size_t i = 0; i < size_; ++i)
      if (buf_[(begin_ + i) % N] != o.buf_[(o.begin_ + i) % N])
        return false;
    return true;
  }
  constexpr auto operator<=>(const FixedRing& o) const {
    for (std::size_t i = 0; i < size_ && i < o.size_; ++i) {
      auto lhs = buf_[(begin_ + i) % N];
      auto rhs = o.buf_[(o.begin_ + i) % N];
      if (auto cmp = lhs <=> rhs; cmp != 0)
        return cmp;
    }
    return size_ <=> o.size_;
  }
};

constexpr bool test_queue() {
  std::queue<int, FixedRing<int, 8>> q;
  if (!q.empty() || q.size() != 0)
    return false;
  q.push(1);
  q.emplace(2);
  std::array<int, 2> more{3, 4};
  q.push_range(more);
  if (q.front() != 1 || q.back() != 4 || q.size() != 4)
    return false;
  std::queue<int, FixedRing<int, 8>> copy(q);
  if (!(copy == q) || !(copy >= q))
    return false;
  copy.pop();
  if (!(copy > q))
    return false;
  copy.swap(q);
  return copy.front() == 1 && q.front() == 2;
}
static_assert(test_queue());

constexpr bool test_queue_constructors() {
  FixedRing<int, 4> ring;
  ring.push_back(1);
  ring.push_back(2);
  std::queue deduced(ring);
  std::queue<int, FixedRing<int, 4>> moved(std::move(deduced));
  return moved.front() == 1 && moved.back() == 2 && moved.size() == 2;
}
static_assert(test_queue_constructors());

int main(int, char**) { return 0; }
