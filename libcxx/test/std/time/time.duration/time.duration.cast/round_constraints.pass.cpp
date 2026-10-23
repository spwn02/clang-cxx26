//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14

// round's target must be a duration with a non-floating-point rep.

#include <chrono>
#include <type_traits>

template <class D, class = void>
struct can_round_duration : std::false_type {};

template <class D>
struct can_round_duration<D, std::void_t<decltype(std::chrono::round<D>(std::chrono::milliseconds{}))>>
    : std::true_type {};

template <class D, class = void>
struct can_round_time_point : std::false_type {};

template <class D>
struct can_round_time_point<D,
                            std::void_t<decltype(std::chrono::round<D>(std::chrono::system_clock::time_point{}))>>
    : std::true_type {};

static_assert(can_round_duration<std::chrono::seconds>::value, "");
static_assert(!can_round_duration<int>::value, "");
static_assert(!can_round_duration<std::chrono::duration<double>>::value, "");
static_assert(can_round_time_point<std::chrono::seconds>::value, "");
static_assert(!can_round_time_point<int>::value, "");
static_assert(!can_round_time_point<std::chrono::duration<double>>::value, "");

int main(int, char**) { return 0; }
