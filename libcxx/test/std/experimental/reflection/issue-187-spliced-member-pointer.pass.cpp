//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

// Upstream bloomberg/clang-p2996#187: a member-pointer declarator whose
// class is a splice in an expansion statement had a dependent splice
// qualifier, but was marked sugared despite being its own canonical type.
// Assertions therefore aborted in ASTContext::getMemberPointerType; without
// assertions, recursive canonicalization never terminated.

#include <meta>

namespace issue_187 {
struct base {};
struct derived : base {
  static inline int calls = 0;
  void test() { ++calls; }
};
} // namespace issue_187

int main(int, char**) {
  constexpr auto context = std::meta::access_context::current();
  template for (constexpr auto entity :
                std::define_static_array(std::meta::members_of(^^issue_187, context))) {
    if constexpr (std::meta::is_class_type(entity)) {
      template for (constexpr auto parent :
                    std::define_static_array(std::meta::bases_of(entity, context))) {
        if constexpr (std::meta::display_string_of(parent) == "base") {
          [:entity:] object;
          template for (constexpr auto member :
                        std::define_static_array(std::meta::members_of(entity, context))) {
            if constexpr (std::meta::has_identifier(member) &&
                          std::meta::identifier_of(member) == "test") {
              void ([:entity:]::*pointer)() = &[:member:];
              (object.*pointer)();
            }
          }
        }
      }
    }
  }
  return issue_187::derived::calls == 1 ? 0 : 1;
}
