//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

// Upstream bloomberg/clang-p2996#212: instantiating this recursive optional
// extraction in a consteval block used to reach the NRVO checker with a
// ConstevalBlockDecl context and assert that the context was a function/block.

#include <meta>
#include <optional>
#include <string_view>

namespace issue_212_scope {
namespace nested {
struct target {};
}
} // namespace issue_212_scope

namespace issue_212 {
template <const char* Name, std::meta::info Scope = ^^issue_212_scope>
consteval std::optional<std::meta::info> find_type() {
  constexpr auto context = std::meta::access_context::current();
  constexpr auto members =
      std::define_static_array(std::meta::members_of(Scope, context));
  template for (constexpr std::meta::info member : members) {
    if constexpr (std::meta::is_namespace(member)) {
      if constexpr (auto found = find_type<Name, member>(); true)
        return found;
    } else if constexpr (std::meta::is_type(member) &&
                         std::meta::has_identifier(member)) {
      constexpr std::string_view identifier =
          std::define_static_string(std::meta::identifier_of(member));
      if constexpr (identifier == Name)
        return member;
    }
  }
  return std::nullopt;
}
} // namespace issue_212

constexpr auto found = issue_212::find_type<std::define_static_string("target")>();
static_assert(found.has_value());
static_assert(std::meta::is_type(*found));

int main(int, char**) { return 0; }
