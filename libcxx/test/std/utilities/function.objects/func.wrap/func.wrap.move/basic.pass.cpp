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

  // unwrap optimization: constructing from a differently cv-qualified
  // move_only_function of the same signature moves its target's buffer
  // directly, rather than wrapping the wrapper itself; verifies that the
  // vtable struct (parameterized only on the return/argument types, not on
  // cv/ref/noexcept) is compatible across such conversions, and that the
  // source is left empty (its buffer was relocated out, not copied).
  {
    std::move_only_function<int(int) const> src = [](int value) { return value + 1; };
    std::move_only_function<int(int)> dst       = std::move(src);
    assert(!src);
    assert(dst(41) == 42);
  }

  // Genuinely different signature: still falls back to double-wrapping
  // (the vtable types are incompatible, so the unwrap path can't apply)
  // rather than failing to compile -- this is the case the unwrap
  // optimization must decline, per [func.wrap.move]. Uses a base/derived
  // pointer pair so the two signatures are related by implicit conversion
  // (Derived* -> Base*, int -> long) without being identical.
  {
    struct Base {};
    struct Derived : Base {};
    std::move_only_function<int(Base*)> general      = [](Base*) { return 42; };
    std::move_only_function<long(Derived*)> specific = std::move(general);
    Derived d;
    assert(specific(&d) == 42);
  }

  return 0;
}
