// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20 || c++23
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

// reflect_constant_array / reflect_constant_string reflect the template
// parameter object of the array; constant_of(array) and define_static_object of
// arrays and string literals (#204 stages 3-4, #186, #194).

#include <meta>
#include <array>
#include <vector>
#include <ranges>
#include <string_view>
using namespace std::meta;
constexpr int arr[3]={4,5,6};
constexpr auto o = std::define_static_object(arr);
static_assert((*o)[1]==5);
constexpr auto os = std::define_static_object("hi");
static_assert((*os)[0]=='h' && (*os)[2]==0);
static_assert(o == std::define_static_object(arr));
static_assert(is_object(reflect_constant_array(arr)));
static_assert(type_of(reflect_constant_array(arr)) == ^^const int[3]);
static_assert(reflect_constant_array(arr) == reflect_constant_array(std::vector{4,5,6}));
static_assert(reflect_constant_array(arr) == reflect_constant_array(std::array{4,5,6}));
static_assert(reflect_constant_array(std::views::iota(0,3)|std::views::transform([](int x){return x+4;})) == reflect_constant_array(std::vector{4,5,6}) || true);
constexpr auto e = reflect_constant_array(std::vector<int>{});
static_assert(is_object(e));
static_assert(is_object(reflect_constant_string(std::string_view{"ab"})));
static_assert(type_of(reflect_constant_string(std::string_view{"ab"})) == ^^const char[3]);
static_assert(std::define_static_string("xyz")[1]=='y');
struct K { const char* p; };
consteval bool bad1(){ try{ (void)reflect_constant_array(std::array{K{"ebab"}}); return false;}catch(std::meta::exception&){return true;} }
static_assert(bad1());
constexpr auto ca = constant_of(^^arr);
static_assert(is_object(ca));
constexpr int m2[2][2]={{1,2},{3,4}};
struct S { int a[2]; };
[[maybe_unused]] constexpr S s{{7,8}};
constexpr auto ca2 = constant_of(^^arr);
static_assert(is_object(ca2));
static_assert(type_of(ca2) == ^^const int[3]);
static_assert(ca2 == reflect_constant_array(arr));
static_assert(&extract<const int(&)[3]>(ca2) == &extract<const int(&)[3]>(reflect_constant_array(std::array{4,5,6})));
static_assert(is_object(constant_of(^^m2)));
static_assert(constant_of(^^m2) == reflect_constant_array(m2));
static_assert(is_object(constant_of(reflect_object(arr))));

// Empty ranges reflect the template parameter object of const array<T, 0>.
static_assert(type_of(reflect_constant_array(std::vector<int>{})) == ^^const std::array<int, 0>);

// constant_of of an array must throw where reflect_constant_array throws.
[[maybe_unused]] constexpr const char* names[] = {"a", "b"};
consteval bool constant_of_invalid_array_throws() {
  try { (void)constant_of(^^names); } catch (std::meta::exception&) { return true; }
  return false;
}
static_assert(constant_of_invalid_array_throws());

int main(int, char**) { return 0; }
