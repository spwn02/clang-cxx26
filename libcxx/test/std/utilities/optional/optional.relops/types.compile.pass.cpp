//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14

// <optional>

// Confirms that optional<T>'s relational operators are Constraints (SFINAE-soft),
// per P2944R3's [optional.relops] "change all the Mandates to Constraints" — not
// hard errors when the contained/referenced type doesn't support the comparison.
// Also covers optional<T&> (this fork's own P2988R11 extension, not present
// upstream when P2944R3 landed), since a reference specialization's declval
// collapsing could plausibly behave differently from the value specialization.

#include <optional>

#include "test_comparisons.h"

template <class T>
using Opt = std::optional<T>;

static_assert(HasOperatorEqual<Opt<int>>);
static_assert(HasOperatorNotEqual<Opt<int>>);
static_assert(HasOperatorLessThan<Opt<int>>);
static_assert(HasOperatorLessThanEqual<Opt<int>>);
static_assert(HasOperatorGreaterThan<Opt<int>>);
static_assert(HasOperatorGreaterThanEqual<Opt<int>>);

static_assert(!HasOperatorEqual<Opt<NonComparable>>);
static_assert(!HasOperatorNotEqual<Opt<NonComparable>>);
static_assert(!HasOperatorLessThan<Opt<NonComparable>>);
static_assert(!HasOperatorLessThanEqual<Opt<NonComparable>>);
static_assert(!HasOperatorGreaterThan<Opt<NonComparable>>);
static_assert(!HasOperatorGreaterThanEqual<Opt<NonComparable>>);

// optional<T&> — this fork's P2988R11 extension.
static_assert(HasOperatorEqual<Opt<int&>>);
static_assert(HasOperatorNotEqual<Opt<int&>>);
static_assert(HasOperatorLessThan<Opt<int&>>);
static_assert(HasOperatorLessThanEqual<Opt<int&>>);
static_assert(HasOperatorGreaterThan<Opt<int&>>);
static_assert(HasOperatorGreaterThanEqual<Opt<int&>>);

static_assert(!HasOperatorEqual<Opt<NonComparable&>>);
static_assert(!HasOperatorNotEqual<Opt<NonComparable&>>);
static_assert(!HasOperatorLessThan<Opt<NonComparable&>>);
static_assert(!HasOperatorLessThanEqual<Opt<NonComparable&>>);
static_assert(!HasOperatorGreaterThan<Opt<NonComparable&>>);
static_assert(!HasOperatorGreaterThanEqual<Opt<NonComparable&>>);

// [optional.comp.with.t] — comparison with a bare T (not another optional).
static_assert(HasOperatorEqual<Opt<int>, int>);
static_assert(!HasOperatorEqual<Opt<NonComparable>, NonComparable>);
static_assert(HasOperatorEqual<Opt<int&>, int>);
static_assert(!HasOperatorEqual<Opt<NonComparable&>, NonComparable>);
