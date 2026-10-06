//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: no-threads

// <hazard_pointer>

// P3428R4: void make_hazard_pointer_batch(span<hazard_pointer>); void clear_hazard_pointer_batch(span<hazard_pointer>) noexcept;

#include <array>
#include <atomic>
#include <cassert>
#include <hazard_pointer>
#include <span>
#include <utility>

#ifndef __cpp_lib_hazard_pointer
#  error __cpp_lib_hazard_pointer should be defined
#endif
#if __cpp_lib_hazard_pointer != 202606L
#  error __cpp_lib_hazard_pointer should have the value 202606L
#endif

static_assert(noexcept(std::clear_hazard_pointer_batch(std::declval<std::span<std::hazard_pointer>>())));
static_assert(!noexcept(std::make_hazard_pointer_batch(std::declval<std::span<std::hazard_pointer>>())));

struct node : std::hazard_pointer_obj_base<node> {
  static inline int destroyed = 0;
  ~node() { ++destroyed; }
};

int main(int, char**) {
  std::array<std::hazard_pointer, 4> batch;
  for (auto& h : batch)
    assert(h.empty());

  // an empty span is fine
  std::make_hazard_pointer_batch(std::span<std::hazard_pointer>());
  std::clear_hazard_pointer_batch(std::span<std::hazard_pointer>());

  std::make_hazard_pointer_batch(batch);
  for (auto& h : batch)
    assert(!h.empty());

  // elements that already own a hazard pointer are left alone; only the empty ones get new ones
  std::hazard_pointer kept = std::move(batch[1]);
  assert(batch[1].empty());
  std::make_hazard_pointer_batch(batch);
  assert(!batch[1].empty());
  assert(!kept.empty());

  // the batch hazard pointers protect like any other
  std::atomic<node*> source(new node);
  node* protected_node = batch[0].protect(source);
  assert(protected_node == source.load());
  source.store(nullptr, std::memory_order_release);
  protected_node->retire();
  assert(node::destroyed == 0);

  std::clear_hazard_pointer_batch(batch);
  for (auto& h : batch)
    assert(h.empty());
  (new node)->retire(); // triggers a reclamation pass; the first node is not protected any more
  assert(node::destroyed == 2);

  // clearing again, and clearing a mixture, is fine
  std::clear_hazard_pointer_batch(batch);
  std::array<std::hazard_pointer, 2> mixed;
  mixed[1] = std::make_hazard_pointer();
  std::clear_hazard_pointer_batch(mixed);
  assert(mixed[0].empty() && mixed[1].empty());
  return 0;
}
