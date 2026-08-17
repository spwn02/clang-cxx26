//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// <rcu>

#include <cassert>
#include <mutex>
#include <rcu>

struct prefix {
  int value = 0;
};

struct node : prefix, std::rcu_obj_base<node> {
  static inline int destroyed = 0;
  ~node() { ++destroyed; }
};

int main(int, char**) {
  std::rcu_domain domain;
  {
    std::scoped_lock read_lock(domain);
    (new node)->retire({}, domain);
    assert(node::destroyed == 0);
  }
  std::rcu_synchronize(domain);
  assert(node::destroyed == 0);
  std::rcu_barrier(domain);
  assert(node::destroyed == 1);

  std::rcu_retire(new node, {}, domain);
  std::rcu_barrier(domain);
  assert(node::destroyed == 2);
}
