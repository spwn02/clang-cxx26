//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++26
// The deprecated shared_ptr atomic access functions are removed in C++26 (P2869R4).
// ADDITIONAL_COMPILE_FLAGS: -Wno-deprecated-declarations

// <memory>
//
// LWG2980: two shared_ptrs are equivalent for atomic_compare_exchange if they store the same pointer value and
// share ownership, or store the same pointer value and are both empty.

#include <cassert>
#include <memory>

int main(int, char**) {
  // Both empty and both null: equivalent, the exchange succeeds.
  {
    std::shared_ptr<int> current, expected, desired = std::make_shared<int>(1);
    assert(std::atomic_compare_exchange_strong(&current, &expected, desired));
    assert(current == desired);
  }

  // Both empty but storing different pointer values (aliasing constructor of an empty owner): not equivalent.
  {
    int x = 0;
    std::shared_ptr<int> current(std::shared_ptr<int>(), &x);
    std::shared_ptr<int> expected;
    std::shared_ptr<int> desired = std::make_shared<int>(1);
    assert(!std::atomic_compare_exchange_strong(&current, &expected, desired));
    assert(current.get() == &x);
    assert(expected.get() == &x);
  }

  // Both empty and storing the same pointer value: equivalent.
  {
    int x = 0;
    std::shared_ptr<int> current(std::shared_ptr<int>(), &x);
    std::shared_ptr<int> expected(std::shared_ptr<int>(), &x);
    std::shared_ptr<int> desired = std::make_shared<int>(1);
    assert(std::atomic_compare_exchange_strong(&current, &expected, desired));
    assert(current == desired);
  }
  return 0;
}
