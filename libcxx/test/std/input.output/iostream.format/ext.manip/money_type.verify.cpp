//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17
// REQUIRES: localization

#include <iomanip>
#include <string>

void test() {
  int invalid = 0;
  (void)std::get_money(invalid); // expected-error@* {{moneyT must be long double or a specialization of basic_string}}
  (void)std::put_money(invalid); // expected-error@* {{moneyT must be long double or a specialization of basic_string}}
}
