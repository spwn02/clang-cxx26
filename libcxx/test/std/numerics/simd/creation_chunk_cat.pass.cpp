//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <simd>

// [simd.creation]: chunk and cat, for both vec and mask.
//
// chunk has two shapes: chunk<T>(x) (chunk type given directly) and chunk<N>(x) (chunk length --
// [simd.creation]/4-5 defers to the first via resize_t<N, decltype(x)>), and each of those splits
// again on whether x.size() divides evenly by the chunk size: the divisible case returns a plain
// array<T, N>, the remainder case returns a tuple of N full-size pieces plus one differently-sized
// tail piece ([simd.creation]/1-3). This file exercises all four combinations, for both vec and
// mask, checking the *type* of what comes back (array vs. tuple, and the tail's resized element
// type) as a named fact, not just the values.

#include <array>
#include <cassert>
#include <concepts>
#include <simd>
#include <tuple>

int main(int, char**) {
  using vec8 = std::simd::vec<int, 8>;

  vec8 v([](auto i) { return static_cast<int>(i); }); // {0,1,2,...,7}

  // chunk<T>, divisible case: 8 / 2 == 4, no remainder -> array<vec2, 4>.
  {
    using vec2 = std::simd::vec<int, 2>;
    auto pieces = std::simd::chunk<vec2>(v);
    static_assert(std::same_as<decltype(pieces), std::array<vec2, 4>>);
    assert(pieces[0][0] == 0 && pieces[0][1] == 1);
    assert(pieces[3][0] == 6 && pieces[3][1] == 7);
  }

  // chunk<N>, divisible case: same as above, chunk length given instead of chunk type.
  {
    auto pieces = std::simd::chunk<4>(v);
    static_assert(std::same_as<decltype(pieces), std::array<std::simd::vec<int, 4>, 2>>);
    assert(pieces[0][3] == 3 && pieces[1][0] == 4);
  }

  // chunk<T>, remainder case: 8 / 3 == 2 full pieces of size 3, plus one tail of size 2
  // (8 % 3 == 2) -- return type is tuple<vec3, vec3, resize_t<2, vec3>>, not an array.
  {
    using vec3 = std::simd::vec<int, 3>;
    auto pieces = std::simd::chunk<vec3>(v);
    using tail_t = std::simd::resize_t<2, vec3>;
    static_assert(std::same_as<decltype(pieces), std::tuple<vec3, vec3, tail_t>>);
    static_assert(std::tuple_size_v<decltype(pieces)> == 3);
    assert(std::get<0>(pieces)[0] == 0 && std::get<0>(pieces)[2] == 2);
    assert(std::get<1>(pieces)[0] == 3 && std::get<1>(pieces)[2] == 5);
    assert(std::get<2>(pieces)[0] == 6 && std::get<2>(pieces)[1] == 7);
    static_assert(tail_t::size() == 2);
  }

  // chunk on basic_mask -- the parallel overload keyed off sizeof(Tp::value_type) via
  // __mask_enabled, not __vec_enabled. Same divisible/remainder split applies.
  {
    using mask8 = vec8::mask_type;
    mask8 k([](auto i) { return static_cast<int>(i) % 2 == 0; }); // T,F,T,F,T,F,T,F

    using mask4 = std::simd::resize_t<4, mask8>;
    auto pieces = std::simd::chunk<mask4>(k);
    static_assert(std::same_as<decltype(pieces), std::array<mask4, 2>>);
    assert(pieces[0][0] == true && pieces[0][1] == false);
    assert(pieces[1][2] == true && pieces[1][3] == false);

    using mask3 = std::simd::resize_t<3, mask8>;
    auto tail_pieces = std::simd::chunk<mask3>(k);
    using mask_tail_t = std::simd::resize_t<2, mask3>;
    static_assert(std::same_as<decltype(tail_pieces), std::tuple<mask3, mask3, mask_tail_t>>);
    assert(std::get<2>(tail_pieces)[0] == true && std::get<2>(tail_pieces)[1] == false);
  }

  // cat: concatenates same-element-type vecs of (possibly) different abis into one vec whose size
  // is the sum of the inputs' sizes -- exercised with three arguments (not just two), since cat is
  // variadic and a two-argument-only test wouldn't cover the fold in creation.h's __append lambda.
  {
    using vec2 = std::simd::vec<int, 2>;
    using vec3 = std::simd::vec<int, 3>;
    vec2 a([](auto i) { return 10 + static_cast<int>(i); }); // {10,11}
    vec3 b([](auto i) { return 20 + static_cast<int>(i); }); // {20,21,22}
    vec3 c([](auto i) { return 30 + static_cast<int>(i); }); // {30,31,32}

    auto joined = std::simd::cat(a, b, c);
    static_assert(decltype(joined)::size() == 8);
    assert(joined[0] == 10 && joined[1] == 11);
    assert(joined[2] == 20 && joined[4] == 22);
    assert(joined[5] == 30 && joined[7] == 32);
  }

  // cat round-trips with chunk: chunking v into 4 pieces of 2 and re-catting them must reproduce v.
  {
    auto pieces  = std::simd::chunk<2>(v);
    auto rejoined = std::apply([](auto... p) { return std::simd::cat(p...); }, pieces);
    static_assert(decltype(rejoined)::size() == 8);
    for (int i = 0; i < 8; ++i)
      assert(rejoined[i] == v[i]);
  }

  // cat on basic_mask -- parallel to the vec overload, keyed on the shared Bytes rather than a
  // shared element type.
  {
    using mask2 = std::simd::vec<int, 2>::mask_type;
    using mask3 = std::simd::vec<int, 3>::mask_type;
    mask2 ka([](auto i) { return static_cast<int>(i) == 0; }); // T,F
    mask3 kb([](auto i) { return static_cast<int>(i) == 1; }); // F,T,F

    auto joined = std::simd::cat(ka, kb);
    static_assert(decltype(joined)::size() == 5);
    assert(joined[0] == true && joined[1] == false);
    assert(joined[2] == false && joined[3] == true && joined[4] == false);
  }

  return 0;
}
