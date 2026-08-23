//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// class text_encoding
// [text.encoding.members]
//
//   constexpr text_encoding() = default;
//   constexpr explicit text_encoding(string_view enc) noexcept;
//   constexpr text_encoding(id i) noexcept;
//   constexpr id mib() const noexcept;
//   constexpr const char* name() const noexcept;
//   static consteval text_encoding literal() noexcept;

#include <text_encoding>

#include <cassert>
#include <string_view>
#include <type_traits>
#include <utility>

#include "test_macros.h"

using TE = std::text_encoding;
using ID = TE::id;

static_assert(TE::max_name_length == 63);

static_assert(std::is_default_constructible_v<TE>);
static_assert(std::is_trivially_copyable_v<TE>);

// text_encoding(string_view) is explicit.
static_assert(std::is_constructible_v<TE, std::string_view>);
static_assert(!std::is_convertible_v<std::string_view, TE>);

// text_encoding(id) is a converting constructor.
static_assert(std::is_convertible_v<ID, TE>);

static_assert(noexcept(TE()));
static_assert(noexcept(TE(std::declval<std::string_view>())));
static_assert(noexcept(TE(std::declval<ID>())));
static_assert(noexcept(std::declval<const TE&>().mib()));
static_assert(noexcept(std::declval<const TE&>().name()));

constexpr bool testDefault() {
  TE e;
  assert(e.mib() == ID::unknown);
  assert(e.name() != nullptr);
  assert(*e.name() == '\0'); // [text.encoding.general]p6 does not apply (mib() == unknown)
  return true;
}

constexpr bool testFromId() {
  TE e(ID::ASCII);
  assert(e.mib() == ID::ASCII);
  // Postconditions (id ctor): mib_ == unknown || mib_ == other implies strlen(name_) == 0;
  // otherwise the alias set contains name_.
  assert(!std::string_view(e.name()).empty());

  TE unk(ID::unknown);
  assert(unk.mib() == ID::unknown);
  assert(std::string_view(unk.name()).empty());

  TE oth(ID::other);
  assert(oth.mib() == ID::other);
  assert(std::string_view(oth.name()).empty());
  return true;
}

constexpr bool testFromName() {
  // "US-ASCII" is the primary IANA name; "ASCII" is a known alias.
  TE a("US-ASCII");
  assert(a.mib() == ID::ASCII);

  TE b("ASCII");
  assert(b.mib() == ID::ASCII);

  // comp-name ignores non-alphanumerics and case, per [text.encoding.members]p19.
  TE c("u.s-ascii");
  assert(c.mib() == ID::ASCII);

  // Unknown/unregistered names resolve to `other`, preserving the exact spelling.
  TE d("not-a-real-encoding-9000");
  assert(d.mib() == ID::other);
  assert(std::string_view(d.name()) == "not-a-real-encoding-9000");
  return true;
}

// [text.encoding.general]p6: an object `e` whose mib() is neither `unknown` nor
// `other` satisfies `*e.name() == '\0'` is false, and `e.mib() ==
// text_encoding(e.name()).mib()` is true (round-trip through the name).
constexpr bool testGeneralInvariant() {
  for (ID i : {ID::ASCII, ID::UTF8, ID::ISOLatin1}) {
    TE e(i);
    assert(*e.name() != '\0');
    assert(e.mib() == TE(e.name()).mib());
  }
  return true;
}

// [text.encoding.members] comp-name examples, exercised indirectly via the
// string_view constructor (comp-name itself is exposition-only).
constexpr bool testCompNameExamples() {
  assert(TE("UTF-8").mib() == ID::UTF8);
  assert(TE("utf8").mib() == ID::UTF8);
  assert(TE("u.t.f-008").mib() == ID::UTF8);
  assert(TE("ut8").mib() != ID::UTF8);
  assert(TE("utf-80").mib() != ID::UTF8);
  return true;
}

int main(int, char**) {
  testDefault();
  static_assert(testDefault());

  testFromId();
  static_assert(testFromId());

  testFromName();
  static_assert(testFromName());

  testGeneralInvariant();
  static_assert(testGeneralInvariant());

  testCompNameExamples();
  static_assert(testCompNameExamples());

  // literal() is consteval.
  constexpr TE lit = TE::literal();
  (void)lit;

  return 0;
}
