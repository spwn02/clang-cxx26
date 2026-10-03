// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20 || c++23
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

#include <meta>
#include <type_traits>
using namespace std::meta;

consteval bool rejects(info type, data_member_options options) {
  try {
    (void)data_member_spec(type, options);
  } catch (exception& e) {
    return e.from() == ^^data_member_spec;
  }
  return false;
}
void function();
int object;
[[maybe_unused]] constexpr int array[] = {1};
[[maybe_unused]] constexpr int constant = 7;
enum E { value };
struct Incomplete;
using Alias = int;
static_assert(std::is_aggregate_v<data_member_options>);
static_assert(rejects(^^void, {.name="x"}));
static_assert(rejects(^^decltype(function), {.name="x"}));
static_assert(rejects(^^object, {.name="x"}));
static_assert(rejects(info{}, {.name="x"}));
static_assert(rejects(^^int, {.name=""}));
static_assert(rejects(^^int, {.name="if"}));
static_assert(rejects(^^int, {.name="true"}));
static_assert(rejects(^^int, {.name="1bad"}));
static_assert(rejects(^^int, {.name=R"(a\u03B1)"}));
static_assert(rejects(^^int, {.name=u8R"(\u03B1)"}));
static_assert(rejects(^^int, {}));
static_assert(rejects(^^int, {.bit_width=3, .annotations={reflect_constant(1)}}));
static_assert(rejects(^^float, {.name="x", .bit_width=3}));
static_assert(rejects(^^int, {.name="x", .alignment=0, .bit_width=0}));
static_assert(rejects(^^int, {.alignment=8, .bit_width=0}));
static_assert(rejects(^^int, {.bit_width=0, .no_unique_address=true}));
static_assert(rejects(^^int, {.name="x", .bit_width=3, .no_unique_address=true}));
static_assert(rejects(^^int, {.bit_width=-1}));
static_assert(rejects(^^int, {.name="x", .bit_width=0}));
static_assert(rejects(^^const int, {.bit_width=3}));
static_assert(rejects(^^volatile int, {.bit_width=0}));
static_assert(rejects(^^int, {.name="x", .alignment=0}));
static_assert(rejects(^^int, {.name="x", .alignment=-4}));
static_assert(rejects(^^int, {.name="x", .alignment=3}));
static_assert(rejects(^^int, {.name="x", .alignment=1}));
static_assert(rejects(^^Incomplete, {.name="x", .alignment=8}));
static_assert(rejects(^^int, {.name="x", .annotations={info{}}}));
static_assert(rejects(^^int, {.name="x", .annotations={^^int}}));
static_assert(rejects(^^int, {.name="x", .annotations={^^function}}));
static_assert(rejects(^^int, {.name="x", .annotations={^^array}}));
static_assert(rejects(^^int, {.name="x", .annotations={^^object}}));
static_assert(is_data_member_spec(data_member_spec(^^int&, {.name="ref"})));
static_assert(is_data_member_spec(data_member_spec(^^Incomplete, {.name="incomplete"})));
static_assert(is_data_member_spec(data_member_spec(^^E, {.name="e", .bit_width=3})));
static_assert(is_data_member_spec(data_member_spec(^^const int, {.name="c", .bit_width=3})));
static_assert(is_data_member_spec(data_member_spec(^^int, {.bit_width=0})));
static_assert(is_data_member_spec(data_member_spec(^^int, {.bit_width=3})));
static_assert(is_data_member_spec(data_member_spec(^^int, {.name=u8"α"})));
static_assert(is_data_member_spec(data_member_spec(^^int, {.name="aα"})));
static_assert(is_data_member_spec(data_member_spec(^^int, {.name="x", .alignment=alignof(int),
                                                        .no_unique_address=true,
                                                        .annotations={reflect_constant(1)}})));
static_assert(data_member_spec(^^Alias, {.name="x"}) == data_member_spec(^^int, {.name="x"}));
struct Annotated;
consteval { define_aggregate(^^Annotated, {data_member_spec(^^int, {.name="x", .annotations={^^constant}})}); }
static_assert(extract<int>(annotations_of(^^Annotated::x)[0]) == 7);
int main() {}
