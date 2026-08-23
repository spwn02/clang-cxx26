//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

// class text_encoding::aliases_view
// [text.encoding.aliases]
//
//   constexpr aliases_view aliases() const noexcept;

#include <text_encoding>

#include <algorithm>
#include <cassert>
#include <ranges>
#include <string_view>
#include <type_traits>

using TE  = std::text_encoding;
using AV  = TE::aliases_view;
using ID  = TE::id;

static_assert(std::copyable<AV>);
static_assert(std::ranges::view<AV>);
static_assert(std::ranges::random_access_range<AV>);
static_assert(std::ranges::borrowed_range<AV>);
static_assert(std::same_as<std::ranges::range_value_t<AV>, const char*>);
static_assert(std::same_as<std::ranges::range_reference_t<AV>, const char*>);

constexpr bool testKnownEncoding() {
  TE ascii(ID::ASCII);
  auto av = ascii.aliases();

  // Non-empty for a known registered character encoding.
  assert(!av.empty());

  // Every element is a non-null, non-empty ntbs (p8).
  for (const char* alias : av) {
    assert(alias != nullptr);
    assert(alias[0] != '\0');
  }

  // The set of aliases for US-ASCII includes "ASCII" ([text.encoding.general]p4).
  assert(std::ranges::find_if(av, [](const char* a) { return std::string_view(a) == "ASCII"; }) != av.end());

  // No duplicate values when compared with strcmp (p1.3).
  for (auto it = av.begin(); it != av.end(); ++it) {
    for (auto it2 = it + 1; it2 != av.end(); ++it2) {
      assert(std::string_view(*it) != std::string_view(*it2));
    }
  }
  return true;
}

// Otherwise, r is an empty range (p1, "Otherwise").
constexpr bool testUnknownEncoding() {
  TE unk;
  assert(unk.aliases().empty());

  TE other("not-a-real-encoding-9000");
  assert(other.aliases().empty());
  return true;
}

// Random-access, borrowed range operations exercised end-to-end.
constexpr bool testRandomAccess() {
  TE ascii(ID::ASCII);
  auto av = ascii.aliases();
  auto n  = std::ranges::distance(av);
  assert(n > 0);
  assert(av.begin() + n == av.end());
  assert(av[0] == *av.begin());
  return true;
}

int main(int, char**) {
  testKnownEncoding();
  static_assert(testKnownEncoding());

  testUnknownEncoding();
  static_assert(testUnknownEncoding());

  testRandomAccess();
  static_assert(testRandomAccess());

  return 0;
}
