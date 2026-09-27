//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03

#include <chrono>
#include <ratio>
#include <type_traits>

using namespace std::chrono;
using decades = duration<int, std::ratio_multiply<std::ratio<10>, years::period>>;

// LWG 3260: conversions to years select the years overload when appropriate.
template<class T>
concept accepts_decades_addition = requires(T value, decades d) {
    value += d;
};

static_assert(accepts_decades_addition<year_month_day>,
              "LWG 3260: year_month_day += a duration convertible to years must be valid");

int main() {
    year_month_day date = 2001y / January / 1d;
    date += decades(1);
    if (date.year() != 2011y || date.month() != January || date.day() != 1d)
        return 1;
}
