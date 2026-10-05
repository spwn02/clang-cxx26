//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// <memory>

// template<class T> constexpr void start_lifetime(T& r) noexcept;
//
// Mandates: T is a complete type and an implicit-lifetime aggregate type.

#include <memory>
#include <string>

struct NotAggregate {
  NotAggregate() {}
  int a;
};

struct NotImplicitLifetime {
  ~NotImplicitLifetime() {}
  int a;
};

// expected-note@*:* 0+ {{in instantiation of}}

void test() {
  int scalar = 0;
  std::string s;
  NotAggregate na;
  NotImplicitLifetime nil;
  std::start_lifetime(scalar); // expected-error@*:* {{std::start_lifetime(r) requires an implicit-lifetime aggregate type}}
  std::start_lifetime(s);      // expected-error@*:* {{std::start_lifetime(r) requires an implicit-lifetime aggregate type}}
  std::start_lifetime(na);     // expected-error@*:* {{std::start_lifetime(r) requires an implicit-lifetime aggregate type}}
  std::start_lifetime(nil);    // expected-error@*:* {{std::start_lifetime(r) requires an implicit-lifetime aggregate type}}
}
