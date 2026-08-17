//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <simd>

// [simd.loadstore]: partial_load/partial_store carry the actual bounds/mask logic; every other
// name in this clause (unchecked_load/unchecked_store, and the iterator-pair overloads) funnels
// through them per the clause's own "Effects: Equivalent to" wording. This file exercises the
// range form, the (iterator, n) form, and the (iterator, sentinel) form for both load and store, in
// both their masked and unmasked shapes, plus the out-of-bounds truncation behavior partial_load's
// range/mask logic is specifically responsible for (loadstore.h:56).

#include <array>
#include <cassert>
#include <concepts>
#include <simd>
#include <vector>

int main(int, char**) {
  using vec4 = std::simd::vec<int, 4>;

  // partial_load, range form, unmasked: fewer elements than V::size() -- tail lanes must be
  // value-initialized, not left indeterminate ([simd.loadstore]/8's "otherwise T()" wording).
  {
    std::array<int, 2> src{7, 8};
    auto v = std::simd::partial_load<vec4>(src);
    static_assert(std::same_as<decltype(v), vec4>);
    assert(v[0] == 7 && v[1] == 8 && v[2] == 0 && v[3] == 0);
  }

  // partial_load, range form, explicitly masked: a false lane must stay 0 even when the source
  // range has enough elements to cover it -- this is what distinguishes the masked overload from
  // just truncating at range size.
  {
    std::array<int, 4> src{1, 2, 3, 4};
    vec4::mask_type k([](auto i) { return static_cast<int>(i) % 2 == 0; }); // T,F,T,F
    auto v = std::simd::partial_load<vec4>(src, k);
    assert(v[0] == 1 && v[1] == 0 && v[2] == 3 && v[3] == 0);
  }

  // partial_load, default-deduced vec type (no explicit _Vp) -- __load_default_vec picks
  // basic_vec<range_value_t<R>, native_abi<range_value_t<R>>>.
  {
    std::array<int, 4> src{9, 10, 11, 12};
    auto v = std::simd::partial_load(src);
    static_assert(std::same_as<typename decltype(v)::value_type, int>);
    assert(v[0] == 9 && decltype(v)::size() == 4);
  }

  // partial_load, (iterator, n) form -- n smaller than the container, and smaller than V::size().
  {
    std::vector<int> src{20, 21, 22, 23, 24};
    auto v = std::simd::partial_load<vec4>(src.begin(), 3);
    assert(v[0] == 20 && v[1] == 21 && v[2] == 22 && v[3] == 0);
  }
  // partial_load, (iterator, n) form, masked.
  {
    std::vector<int> src{30, 31, 32, 33};
    vec4::mask_type k([](auto i) { return static_cast<int>(i) < 2; }); // T,T,F,F
    auto v = std::simd::partial_load<vec4>(src.begin(), 4, k);
    assert(v[0] == 30 && v[1] == 31 && v[2] == 0 && v[3] == 0);
  }

  // partial_load, (iterator, sentinel) form -- distance computed from the sentinel, not passed
  // directly, exercising the sized_sentinel_for overload separately from the (iterator, n) one.
  {
    std::vector<int> src{40, 41, 42};
    auto v = std::simd::partial_load<vec4>(src.begin(), src.end());
    assert(v[0] == 40 && v[1] == 41 && v[2] == 42 && v[3] == 0);
  }

  // unchecked_load: [simd.loadstore]/1-5, Effects: Equivalent to partial_load<V>(r, mask, f) when
  // ranges::size(r) >= V::size() -- exercised at exactly V::size() so the "unchecked" contract
  // (caller guarantees enough elements) is satisfied, and checked against all three shapes
  // (range, iterator+n, iterator+sentinel), masked and unmasked.
  {
    std::array<int, 4> src{50, 51, 52, 53};
    auto v = std::simd::unchecked_load<vec4>(src);
    assert(v[0] == 50 && v[3] == 53);

    vec4::mask_type k([](auto i) { return static_cast<int>(i) != 1; }); // T,F,T,T
    auto vm = std::simd::unchecked_load<vec4>(src, k);
    assert(vm[0] == 50 && vm[1] == 0 && vm[2] == 52 && vm[3] == 53);

    auto vi = std::simd::unchecked_load<vec4>(src.begin(), 4);
    assert(vi[2] == 52);
    auto vis = std::simd::unchecked_load<vec4>(src.begin(), src.end());
    assert(vis[2] == 52);
  }

  // partial_store, range form, masked -- only the masked-in lanes may write; the false lane's
  // pre-existing value in the destination must survive untouched.
  {
    std::array<int, 4> dst{-1, -1, -1, -1};
    vec4 v([](auto i) { return static_cast<int>(i) + 100; }); // {100,101,102,103}
    vec4::mask_type k([](auto i) { return static_cast<int>(i) % 2 == 1; }); // F,T,F,T
    std::simd::partial_store(v, dst, k);
    assert(dst[0] == -1 && dst[1] == 101 && dst[2] == -1 && dst[3] == 103);
  }

  // partial_store, range form, unmasked, destination narrower than V::size() -- only the
  // in-bounds lanes get written, the excess source lanes are simply dropped (Preconditions in the
  // clause govern this, but the implementation truncates safely rather than trapping).
  {
    std::array<int, 2> dst{-1, -1};
    vec4 v([](auto i) { return static_cast<int>(i) + 200; });
    std::simd::partial_store(v, dst);
    assert(dst[0] == 200 && dst[1] == 201);
  }

  // partial_store, (iterator, n) and (iterator, sentinel) forms, masked and unmasked.
  {
    std::vector<int> dst(4, -1);
    vec4 v([](auto i) { return static_cast<int>(i) + 300; });
    std::simd::partial_store(v, dst.begin(), 4);
    assert(dst[0] == 300 && dst[3] == 303);

    std::vector<int> dst2(4, -1);
    vec4::mask_type k([](auto i) { return static_cast<int>(i) == 0 || static_cast<int>(i) == 3; }); // T,F,F,T
    std::simd::partial_store(v, dst2.begin(), 4, k);
    assert(dst2[0] == 300 && dst2[1] == -1 && dst2[2] == -1 && dst2[3] == 303);

    std::vector<int> dst3(4, -1);
    std::simd::partial_store(v, dst3.begin(), dst3.end());
    assert(dst3[1] == 301);
  }

  // unchecked_store: Effects: Equivalent to partial_store(v, r, mask, f) -- exercised at exactly
  // V::size() so the "unchecked" contract is satisfied, across all three destination shapes.
  {
    std::array<int, 4> dst{-1, -1, -1, -1};
    vec4 v([](auto i) { return static_cast<int>(i) + 400; });
    std::simd::unchecked_store(v, dst);
    assert(dst[0] == 400 && dst[3] == 403);

    std::array<int, 4> dst2{-1, -1, -1, -1};
    vec4::mask_type k([](auto i) { return static_cast<int>(i) < 2; });
    std::simd::unchecked_store(v, dst2, k);
    assert(dst2[0] == 400 && dst2[1] == 401 && dst2[2] == -1 && dst2[3] == -1);

    std::array<int, 4> dst3{-1, -1, -1, -1};
    std::simd::unchecked_store(v, dst3.begin(), 4);
    assert(dst3[2] == 402);

    std::array<int, 4> dst4{-1, -1, -1, -1};
    std::simd::unchecked_store(v, dst4.begin(), dst4.end());
    assert(dst4[2] == 402);
  }

  return 0;
}
