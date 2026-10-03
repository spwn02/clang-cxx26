// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20
// ADDITIONAL_COMPILE_FLAGS: -freflection

#include <meta>
#include <string_view>

namespace ns {}

consteval bool descriptive(auto call) {
  try {
    call();
  } catch (const std::meta::exception& e) {
    std::string_view message(e.what());
    return !message.empty() && message != "bad" && !message.starts_with("bad ");
  }
  return false;
}

static_assert(descriptive([] { std::meta::size_of(^^void); }));
static_assert(descriptive([] { std::meta::bit_size_of(^^void); }));
static_assert(descriptive([] { std::meta::alignment_of(^^void); }));
static_assert(descriptive([] { std::meta::offset_of(^^int); }));
static_assert(descriptive([] { std::meta::operator_of(^^int); }));
static_assert(descriptive([] { std::meta::symbol_of(static_cast<std::meta::operators>(0)); }));
static_assert(descriptive([] { std::meta::template_of(^^int); }));
static_assert(descriptive([] { std::meta::enumerators_of(^^int); }));
static_assert(descriptive([] { std::meta::subobjects_of(^^int, std::meta::access_context::unchecked()); }));
static_assert(descriptive([] { std::meta::bases_of(^^ns, std::meta::access_context::unchecked()); }));
static_assert(descriptive([] { std::meta::make_signed(^^bool); }));
static_assert(descriptive([] { std::meta::tuple_size(^^int); }));

int main(int, char**) { return 0; }
