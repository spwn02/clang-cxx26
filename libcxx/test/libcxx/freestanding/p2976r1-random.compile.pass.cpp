//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23
// ADDITIONAL_COMPILE_FLAGS: -ffreestanding -fno-exceptions -D_LIBCPP_HAS_NO_THREADS

#include <random>
#include <version>

static_assert(__cpp_lib_freestanding_random == 202502L);

void test_freestanding_random_surface() {
  std::minstd_rand0 first;
  std::minstd_rand second;
  std::mt19937 third;
  std::mt19937_64 fourth;
  std::ranlux24_base fifth;
  std::ranlux48_base sixth;
  std::ranlux24 seventh;
  std::ranlux48 eighth;
  std::linear_congruential_engine<unsigned, 16807, 0, 2147483647> ninth;
  std::mersenne_twister_engine<unsigned, 32, 624, 397, 31, 0x9908b0df, 11, 0xffffffff, 7, 0x9d2c5680,
                               15, 0xefc60000, 18, 1812433253>
      tenth;
  std::subtract_with_carry_engine<unsigned, 24, 10, 24> eleventh;
  std::discard_block_engine<std::minstd_rand, 4, 2> twelfth;
  std::independent_bits_engine<std::minstd_rand, 16, unsigned> thirteenth;
  std::uniform_int_distribution<int> distribution;

  (void)first();
  (void)second();
  (void)third();
  (void)fourth();
  (void)fifth();
  (void)sixth();
  (void)seventh();
  (void)eighth();
  (void)ninth();
  (void)tenth();
  (void)eleventh();
  (void)twelfth();
  (void)thirteenth();
  (void)distribution(second);
  static_assert(std::uniform_random_bit_generator<std::mt19937>);
}
