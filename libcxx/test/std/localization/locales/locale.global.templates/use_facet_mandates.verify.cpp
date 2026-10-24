//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17
// ADDITIONAL_COMPILE_FLAGS: -Xclang -verify-ignore-unexpected=note

#include <locale>

struct not_a_facet {};
struct faux_facet {
  static std::locale::id id;
};

void test() {
  (void)std::use_facet<not_a_facet>(std::locale()); // expected-error@* {{no member named 'id'}}
  (void)std::use_facet<faux_facet>(std::locale());  // expected-error@* {{no viable conversion}}
}
