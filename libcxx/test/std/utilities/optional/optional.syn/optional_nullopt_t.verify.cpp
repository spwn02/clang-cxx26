//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14

// <optional>

// A program that necessitates the instantiation of template optional for
// (possibly cv-qualified) nullopt_t is ill-formed.

#include <optional>

#include "test_macros.h"

void f() {
    std::optional<std::nullopt_t> opt; // expected-note 1 {{requested here}}
    std::optional<const std::nullopt_t> opt1; // expected-note 1 {{requested here}}
#if TEST_STD_VER >= 26
    // optional<nullopt_t&> routes to the [optional.optional.ref] partial
    // specialization (P2988), which the standard does not exclude nullopt_t
    // from -- unlike the primary template, it has no such restriction.
    std::optional<std::nullopt_t &> opt2;
#else
    std::optional<std::nullopt_t &> opt2; // expected-note 1 {{requested here}}
#endif
    std::optional<std::nullopt_t &&> opt3; // expected-note 1 {{requested here}}
#if TEST_STD_VER >= 26
    // expected-error@optional:* 3 {{instantiation of optional with nullopt_t is ill-formed}}
#else
    // expected-error@optional:* 4 {{instantiation of optional with nullopt_t is ill-formed}}
#endif
}
