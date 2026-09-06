//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17

// template<class U> atomic_ref(const atomic_ref<U>&);

#include <atomic>
#include <cassert>
#include <type_traits>

template <class T>
void test() {
  using Ref      = std::atomic_ref<T>;
  using ConstRef = std::atomic_ref<const T>;

  static_assert(std::is_constructible_v<ConstRef, const Ref&>);
  static_assert(std::is_nothrow_constructible_v<ConstRef, const Ref&>);
  static_assert(!std::is_constructible_v<Ref, const ConstRef&>);

  T value;
  if constexpr (std::is_pointer_v<T>) {
    static int pointed_to = 42;
    value                 = &pointed_to;
  } else {
    value = T(42);
  }
  Ref ref(value);
  ConstRef converted(ref);
  assert(converted.load() == value);
}

int main(int, char**) {
  test<bool>();
  test<int>();
  test<double>();
  test<int*>();

  static_assert(!std::is_constructible_v<std::atomic_ref<int>, const std::atomic_ref<float>&>);
  return 0;
}
