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

// template<class... S> class copyable_function; // not defined
// template<class R, class... ArgTypes>
// class copyable_function<R(ArgTypes...) cv ref noexcept(noex)>;

#include <cassert>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>
#include <version>

struct LargeCallable {
  int storage[16]{};

  int operator()(int value) const { return value + 1; }
};

struct MoveOnlyCallable {
  std::unique_ptr<int> ptr = std::make_unique<int>(0);

  MoveOnlyCallable()                                    = default;
  MoveOnlyCallable(MoveOnlyCallable&&)                  = default;
  MoveOnlyCallable(const MoveOnlyCallable&)             = delete;
  int operator()(int value) const { return value; }
};

struct CopyCounting {
  static int copies;

  CopyCounting()                    = default;
  CopyCounting(const CopyCounting&) { ++copies; }
  int operator()(int value) const { return value + 1; }
};
int CopyCounting::copies = 0;

int main(int, char**) {
  static_assert(__cpp_lib_copyable_function == 202306L);
  static_assert(std::is_copy_constructible_v<std::copyable_function<void()>>);
  static_assert(std::is_nothrow_move_constructible_v<std::copyable_function<void()>>);
  static_assert(!std::is_constructible_v<std::copyable_function<int(int)>, MoveOnlyCallable>);
  static_assert(!std::is_nothrow_constructible_v<std::copyable_function<int(int)>, LargeCallable>);
  static_assert(noexcept(std::declval<std::copyable_function<void() noexcept>&>()()));

  // basic call, small-buffer-optimized storage
  {
    int value                       = 41;
    std::copyable_function<int()> f = [value] { return value + 1; };
    assert(f() == 42);
  }

  // heap-allocated storage (object too large for the inline buffer)
  {
    std::copyable_function<int(int)> f = LargeCallable{};
    assert(f(41) == 42);
  }

  // copy constructor: independent copies, each copy-constructing the target
  {
    CopyCounting::copies                = 0;
    std::copyable_function<int(int)> f1 = CopyCounting{};
    int before                          = CopyCounting::copies;
    std::copyable_function<int(int)> f2 = f1;
    assert(CopyCounting::copies == before + 1);
    assert(f1(1) == 2);
    assert(f2(1) == 2);
  }

  // copy assignment leaves the source untouched
  {
    std::copyable_function<int(int)> f1 = [](int value) { return value + 1; };
    std::copyable_function<int(int)> f2 = [](int value) { return value + 100; };
    f2                                  = f1;
    assert(f2(1) == 2);
    assert(f1(1) == 2);
  }

  // move constructor / move assignment
  {
    std::copyable_function<int()> f     = [] { return 42; };
    std::copyable_function<int()> moved = std::move(f);
    assert(!f);
    assert(moved() == 42);

    std::copyable_function<int()> f2 = [] { return 7; };
    f2                               = std::move(moved);
    assert(!moved);
    assert(f2() == 42);
  }

  // nullptr construction / assignment
  {
    std::copyable_function<int()> f = [] { return 1; };
    f                                = nullptr;
    assert(f == nullptr);
    assert(!f);
  }

  // const-qualified specialization: invocable on a const object
  {
    struct ConstFunctor {
      int operator()() const { return 42; }
    };
    std::copyable_function<int() const> f = ConstFunctor{};
    const auto& cref                      = f;
    assert(cref() == 42);
  }

  // noexcept specialization
  {
    std::copyable_function<int() noexcept> f = []() noexcept { return 42; };
    assert(f() == 42);
  }

  // in_place_type_t construction
  {
    std::copyable_function<int(int)> f{std::in_place_type<LargeCallable>};
    assert(f(41) == 42);
  }

  // in_place_type_t + initializer_list construction
  {
    struct FromInitList {
      int sum;
      FromInitList(std::initializer_list<int> il, int extra) {
        sum = extra;
        for (int v : il)
          sum += v;
      }
      int operator()() const { return sum; }
    };
    std::copyable_function<int()> f{std::in_place_type<FromInitList>, {1, 2, 3}, 10};
    assert(f() == 16);
  }

  // ref-qualified specializations
  {
    std::copyable_function<int() const&> lref = [] { return 42; };
    assert(std::as_const(lref)() == 42);

    std::copyable_function<int()&&> rref = [] { return 42; };
    assert(std::move(rref)() == 42);
  }

  // function pointer construction
  {
    int (*fp)(int)                     = [](int value) { return value + 1; };
    std::copyable_function<int(int)> f = fp;
    assert(f(41) == 42);
  }

  // conversion from std::function (double-wrapping is allowed, just not optimized)
  {
    std::function<int(int)> sf         = [](int value) { return value + 1; };
    std::copyable_function<int(int)> f = sf;
    assert(f(41) == 42);
  }

  // unwrap optimization: constructing from a differently cv-qualified
  // copyable_function of the same signature clones its target directly,
  // rather than wrapping the wrapper itself; verifies that the vtable
  // struct (parameterized only on the return/argument types, not on
  // cv/ref/noexcept) is compatible across such conversions.
  {
    CopyCounting::copies                       = 0;
    std::copyable_function<int(int) const> src = CopyCounting{};
    int before                                 = CopyCounting::copies;
    std::copyable_function<int(int)> dst       = src;
    // Exactly one copy: the target object itself was cloned, not the
    // wrapper — double-wrapping would additionally copy-construct a
    // nested copyable_function<int(int) const>.
    assert(CopyCounting::copies == before + 1);
    assert(dst(41) == 42);
    assert(src(41) == 42);
  }

  // Genuinely different signature: still falls back to double-wrapping
  // (the vtable types are incompatible, so the unwrap path can't apply)
  // rather than failing to compile — this is the case the unwrap
  // optimization must decline, per [func.wrap.copy].
  {
    std::copyable_function<int(int)> src = [](int value) { return value + 1; };
    std::copyable_function<long(short)> dst{src};
    assert(dst(41) == 42);
    assert(src(41) == 42);
  }

  return 0;
}
