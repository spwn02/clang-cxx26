//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// <hazard_pointer>

#include <atomic>
#include <cassert>
#include <hazard_pointer>

struct prefix {
  int value = 0;
};

struct node : prefix, std::hazard_pointer_obj_base<node> {
  static inline int destroyed = 0;

  ~node() { ++destroyed; }
};

int main(int, char**) {
  std::atomic<node*> source(new node);
  std::hazard_pointer hazard = std::make_hazard_pointer();

  node* protected_node = hazard.protect(source);
  assert(protected_node == source.load());
  source.store(nullptr, std::memory_order_release);
  protected_node->retire();
  assert(node::destroyed == 0);

  hazard.reset_protection();
  (new node)->retire();
  assert(node::destroyed == 2);
}
