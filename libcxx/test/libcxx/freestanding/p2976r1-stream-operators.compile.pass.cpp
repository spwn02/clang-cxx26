//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// ADDITIONAL_COMPILE_FLAGS: -ffreestanding -fno-exceptions -D_LIBCPP_HAS_NO_THREADS

// P2976R1: the operator<< and operator>> of the partially freestanding engines and of
// uniform_int_distribution are hosted; a freestanding <random> does not declare them.

#include <random>

template <class T>
concept ostreamable = requires(std::basic_ostream<char>& os, const T& t) { os << t; };
template <class T>
concept istreamable = requires(std::basic_istream<char>& is, T& t) { is >> t; };

using lcg = std::linear_congruential_engine<unsigned, 16807, 0, 2147483647>;
using swc = std::subtract_with_carry_engine<unsigned, 24, 10, 24>;
using dbe = std::discard_block_engine<std::minstd_rand, 4, 2>;
using ibe = std::independent_bits_engine<std::minstd_rand, 16, unsigned>;

static_assert(!ostreamable<lcg> && !istreamable<lcg>);
static_assert(!ostreamable<std::mt19937> && !istreamable<std::mt19937>);
static_assert(!ostreamable<swc> && !istreamable<swc>);
static_assert(!ostreamable<dbe> && !istreamable<dbe>);
static_assert(!ostreamable<ibe> && !istreamable<ibe>);
static_assert(!ostreamable<std::philox4x32> && !istreamable<std::philox4x32>);
static_assert(!ostreamable<std::uniform_int_distribution<int>> && !istreamable<std::uniform_int_distribution<int>>);
