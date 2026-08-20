//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// UNSUPPORTED: gcc

// <functional>

// template<class... S> class function_ref; // not defined
// template<class R, class... ArgTypes>
// class function_ref<R(ArgTypes...) cv noexcept(noex)>;

#include <cassert>
#include <functional>
#include <type_traits>
#include <utility>
#include <version>

static int free_function(int value) { return value + 1; }
static int noexcept_free_function(int value) noexcept { return value + 2; }

struct ConstFunctor {
  int operator()(int value) const { return value * 2; }
};

struct HasMemberFunction {
  int val;
  int get(int add) const { return val + add; }
};

// Constexpr construction: exercises the void* round trip inside
// __function_ref_bound_entity under constant evaluation. operator() itself
// is not constexpr per [func.wrap.ref], so invocation is checked at runtime.
inline constexpr auto GlobalLambda        = [](int value) { return value + 1; };
constexpr std::function_ref<int(int)> ConstexprFR = GlobalLambda;

int main(int, char**) {
  static_assert(__cpp_lib_function_ref == 202306L);

  static_assert(std::is_trivially_copyable_v<std::function_ref<int(int)>>);
  static_assert(std::is_copy_constructible_v<std::function_ref<int(int)>>);

  // Deleted operator=(T) carve-outs: same type, pointer, nontype_t.
  static_assert(std::is_assignable_v<std::function_ref<int(int)>&, std::function_ref<int(int)>>);
  static_assert(std::is_assignable_v<std::function_ref<int(int)>&, decltype(free_function)>); // decays to a pointer
  static_assert(std::is_assignable_v<std::function_ref<int(int)>&, decltype(std::nontype<free_function>)>);
  {
    auto lambda = [](int value) { return value; };
    // Not a pointer, not nontype_t, not function_ref: assignment stays deleted
    // to guard against binding a dangling temporary.
    static_assert(!std::is_assignable_v<std::function_ref<int(int)>&, decltype(lambda)>);
  }

  assert(ConstexprFR(41) == 42);

  // function pointer constructor (not constexpr)
  {
    std::function_ref<int(int)> fr = free_function;
    assert(fr(1) == 2);
  }

  // generic F&& constructor, from a lambda (constexpr-friendly)
  {
    auto lambda                 = [](int value) { return value * 3; };
    std::function_ref<int(int)> fr = lambda;
    assert(fr(2) == 6);
  }

  // cv-qualified specialization, bound to a const object
  {
    const ConstFunctor functor{};
    std::function_ref<int(int) const> fr = functor;
    assert(fr(3) == 6);
  }

  // noexcept specialization
  {
    std::function_ref<int(int) noexcept> fr = noexcept_free_function;
    static_assert(noexcept(fr(1)));
    assert(fr(1) == 3);
  }

  // nontype_t<f> constructor: bind a free function at compile time
  {
    std::function_ref<int(int)> fr{std::nontype<free_function>};
    assert(fr(4) == 5);
  }

  // nontype_t<f>, U&& constructor: bind a pointer-to-member-function to an lvalue object
  {
    HasMemberFunction object{10};
    std::function_ref<int(int)> fr{std::nontype<&HasMemberFunction::get>, object};
    assert(fr(5) == 15);
  }

  // nontype_t<f>, cv T* constructor: bind a pointer-to-member-function to an object pointer
  {
    HasMemberFunction object{100};
    std::function_ref<int(int)> fr{std::nontype<&HasMemberFunction::get>, &object};
    assert(fr(1) == 101);
  }

  // copy semantics: trivially copyable, copies refer to the same target
  {
    auto lambda                  = [](int value) { return value + 1000; };
    std::function_ref<int(int)> fr1 = lambda;
    std::function_ref<int(int)> fr2 = fr1;
    assert(fr2(1) == 1001);
  }

  // operator=(T) carve-outs actually retarget the reference at runtime
  {
    auto lambda                  = [](int) { return -1; };
    std::function_ref<int(int)> fr = lambda;
    fr                            = free_function;
    assert(fr(1) == 2);
    fr = std::nontype<free_function>;
    assert(fr(4) == 5);
  }

  return 0;
}
