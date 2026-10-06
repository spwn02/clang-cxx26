//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <functional>

// [func.wrap.ref.deduct]: deduction guides of function_ref (P3948R1, P0792R14)

#include <cassert>
#include <functional>
#include <type_traits>
#include <utility>

int free_function(int i) { return i + 1; }
int noexcept_free_function(int i) noexcept { return i + 2; }

struct S {
  int data = 7;
  int get() const { return data; }
  int get_ref() & { return data; }
  int noex() const noexcept { return data + 1; }
  int cv(int i) const volatile { return i; }
};

int with_receiver(S&, int i) { return i; }
int noexcept_with_receiver(S*, int i) noexcept { return i; }

// function_ref(F*) -> function_ref<F>
static_assert(std::is_same_v<decltype(std::function_ref(free_function)), std::function_ref<int(int)>>);
static_assert(std::is_same_v<decltype(std::function_ref(&free_function)), std::function_ref<int(int)>>);
static_assert(std::is_same_v<decltype(std::function_ref(noexcept_free_function)), std::function_ref<int(int) noexcept>>);

// function_ref(constant_wrapper<c, F0>) -> function_ref<remove_pointer_t<F0>>
static_assert(std::is_same_v<decltype(std::function_ref(std::cw<free_function>)), std::function_ref<int(int)>>);
static_assert(
    std::is_same_v<decltype(std::function_ref(std::cw<noexcept_free_function>)), std::function_ref<int(int) noexcept>>);

// function_ref(constant_wrapper<c, F>, T&&): member function pointers, data member pointers, functions with a receiver
static_assert(std::is_same_v<decltype(std::function_ref(std::cw<&S::get>, std::declval<S&>())), std::function_ref<int()>>);
static_assert(std::is_same_v<decltype(std::function_ref(std::cw<&S::get_ref>, std::declval<S&>())), std::function_ref<int()>>);
static_assert(
    std::is_same_v<decltype(std::function_ref(std::cw<&S::noex>, std::declval<S&>())), std::function_ref<int() noexcept>>);
static_assert(std::is_same_v<decltype(std::function_ref(std::cw<&S::cv>, std::declval<S&>())), std::function_ref<int(int)>>);
static_assert(
    std::is_same_v<decltype(std::function_ref(std::cw<&S::data>, std::declval<S&>())), std::function_ref<int&() noexcept>>);
static_assert(std::is_same_v<decltype(std::function_ref(std::cw<with_receiver>, std::declval<S&>())), std::function_ref<int(int)>>);
static_assert(std::is_same_v<decltype(std::function_ref(std::cw<noexcept_with_receiver>, std::declval<S*>())),
                             std::function_ref<int(int) noexcept>>);

int main(int, char**) {
  S s;
  std::function_ref ref1(free_function);
  assert(ref1(1) == 2);
  std::function_ref ref2(std::cw<&S::get>, s);
  assert(ref2() == 7);
  std::function_ref ref3(std::cw<&S::data>, s);
  ref3() = 9;
  assert(s.data == 9);
  std::function_ref ref4(std::cw<with_receiver>, s);
  assert(ref4(5) == 5);
  return 0;
}
