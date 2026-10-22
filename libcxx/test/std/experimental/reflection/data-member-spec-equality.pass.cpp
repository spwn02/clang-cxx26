//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23

#include <meta>
using namespace std::meta;

// [class.mem.general]: two descriptions of a non-static data member are equal when all of
// (T, N, A, W, NUA, ANN) are equal.

#include <meta>
using namespace std::meta;

static_assert(data_member_spec(^^int, {.name = "a"}) == data_member_spec(^^int, {.name = "a"}));
static_assert(data_member_spec(^^int, {.name = "a"}) != data_member_spec(^^int, {.name = "b"}));
static_assert(data_member_spec(^^int, {.name = "a", .no_unique_address = true}) !=
              data_member_spec(^^int, {.name = "a"}));
static_assert(data_member_spec(^^int, {.name = "a", .annotations = {reflect_constant(1)}}) ==
              data_member_spec(^^int, {.name = "a", .annotations = {reflect_constant(1)}}));
static_assert(data_member_spec(^^int, {.name = "a", .annotations = {reflect_constant(1)}}) !=
              data_member_spec(^^int, {.name = "a", .annotations = {reflect_constant(2)}}));
static_assert(data_member_spec(^^int, {.name = "a", .annotations = {reflect_constant(1)}}) !=
              data_member_spec(^^int, {.name = "a"}));
static_assert(data_member_spec(^^int, {.name = "a", .annotations = {reflect_constant(1), reflect_constant(2)}}) !=
              data_member_spec(^^int, {.name = "a", .annotations = {reflect_constant(2), reflect_constant(1)}}));
int main(int, char**) { return 0; }
