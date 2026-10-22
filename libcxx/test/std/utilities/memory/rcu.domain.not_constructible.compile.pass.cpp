//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// [saferecl.rcu.domain]: rcu_domain has no constructor but the deleted copy constructor, so a program
// cannot create one; rcu_default_domain() returns the only one.

#include <rcu>
#include <type_traits>

static_assert(!std::is_default_constructible_v<std::rcu_domain>);
static_assert(!std::is_copy_constructible_v<std::rcu_domain>);
static_assert(!std::is_copy_assignable_v<std::rcu_domain>);
