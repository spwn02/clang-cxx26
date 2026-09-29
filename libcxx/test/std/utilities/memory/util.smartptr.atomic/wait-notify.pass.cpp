//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17

#include <atomic>
#include <memory>

int main(int, char**) {
  auto owner = std::make_shared<int>(42);
  std::atomic<std::shared_ptr<int>> shared(owner);
  shared.wait(std::shared_ptr<int>(), std::memory_order_acquire);
  shared.notify_one();
  shared.notify_all();
  std::atomic<std::weak_ptr<int>> weak{std::weak_ptr<int>(owner)};
  weak.wait(std::weak_ptr<int>(), std::memory_order_acquire);
  weak.notify_one();
  weak.notify_all();
  return 0;
}
