//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20
// UNSUPPORTED: gcc

#include <cassert>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>
#include <version>

struct LargeCallable {
  int storage[16]{};

  int operator()(int value) { return value + 1; }
};

int main(int, char**) {
  static_assert(__cpp_lib_move_only_function == 202110L);
  static_assert(!std::is_copy_constructible_v<std::move_only_function<void()>>);
  static_assert(std::is_nothrow_move_constructible_v<std::move_only_function<void()>>);
  static_assert(!std::is_nothrow_constructible_v<std::move_only_function<int(int)>, LargeCallable>);
  static_assert(noexcept(std::declval<std::move_only_function<void() noexcept>&>()()));

  auto pointer                            = std::make_unique<int>(41);
  std::move_only_function<int()> function = [pointer = std::move(pointer)] { return *pointer + 1; };
  assert(function() == 42);

  std::move_only_function<int(int)> large = LargeCallable{};
  assert(large(41) == 42);

  std::move_only_function<int() const & noexcept> qualified = []() noexcept { return 42; };
  assert(std::as_const(qualified)() == 42);

  std::move_only_function<int()> moved = std::move(function);
  assert(!function);
  assert(moved() == 42);

  moved = nullptr;
  assert(moved == nullptr);

  return 0;
}
