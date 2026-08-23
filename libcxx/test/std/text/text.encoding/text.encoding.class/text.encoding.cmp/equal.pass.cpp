//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// class text_encoding
// [text.encoding.cmp]
//
//   friend constexpr bool operator==(const text_encoding& a, const text_encoding& b) noexcept;
//   friend constexpr bool operator==(const text_encoding& encoding, id i) noexcept;

#include <text_encoding>

#include <cassert>
#include <utility>

using TE = std::text_encoding;
using ID = TE::id;

static_assert(noexcept(std::declval<const TE&>() == std::declval<const TE&>()));
static_assert(noexcept(std::declval<const TE&>() == std::declval<ID>()));

// Two `other`-classified encodings compare via comp-name on their stored names,
// not by mib_ alone (p1: "If a.mib_ == other && b.mib_ == other ... comp-name(...)").
constexpr bool testOtherComparesByName() {
  TE a("bogus-encoding-alpha");
  TE b("bogus-encoding-beta");
  TE c("bogus-encoding-alpha"); // same spelling as `a`

  assert(a.mib() == ID::other);
  assert(b.mib() == ID::other);
  assert(a != b); // different names, both `other`
  assert(a == c); // same name, both `other`
  return true;
}

// Otherwise (at least one side isn't `other`), comparison is by mib_ alone.
constexpr bool testKnownComparesByMib() {
  TE a(ID::ASCII);
  TE b("US-ASCII"); // resolves to the same mib via a different spelling
  assert(a == b);

  TE c(ID::UTF8);
  assert(a != c);
  return true;
}

// operator==(text_encoding, id).
constexpr bool testCompareWithId() {
  TE a(ID::ASCII);
  assert(a == ID::ASCII);
  assert(!(a == ID::UTF8));

  // Per p3, this overload does not induce an equivalence relation when i == other:
  // two distinct `other` encodings both compare equal to `id::other`, but not to
  // each other (see testOtherComparesByName above).
  TE b("bogus-encoding-alpha");
  TE c("bogus-encoding-beta");
  assert(b == ID::other);
  assert(c == ID::other);
  assert(b != c);
  return true;
}

int main(int, char**) {
  testOtherComparesByName();
  static_assert(testOtherComparesByName());

  testKnownComparesByMib();
  static_assert(testKnownComparesByMib());

  testCompareWithId();
  static_assert(testCompareWithId());

  return 0;
}
