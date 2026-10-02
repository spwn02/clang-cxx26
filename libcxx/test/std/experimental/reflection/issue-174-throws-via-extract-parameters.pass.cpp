// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20 || c++23
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

// access_context::via throws unless given null or a complete class; extract-ref
// throws for a static-storage variable that is not usable in constant
// expressions; parameters_of(function type) de-aliases parameter types (#174,
// #185, #195).

#include <meta>

using namespace std::meta;

struct C {};
struct Inc;
enum E { e0 };

consteval bool via_throws(info r) {
  try { (void)access_context::current().via(r); } catch (std::meta::exception&) { return true; }
  return false;
}
static_assert(via_throws(^^Inc) && via_throws(^^int) && via_throws(^^::) && via_throws(^^E));
static_assert(!via_throws(^^C) && !via_throws(info{}));
static_assert(access_context::current().via(^^C).designating_class() == ^^C);

struct Self {
  // Self is incomplete in a static data member initializer, so via(^^Self) throws.
  static constexpr bool via_throws_while_incomplete = [] consteval {
    try { (void)access_context::current().via(^^Self); } catch (std::meta::exception&) { return true; }
    return false;
  }();
};
static_assert(Self::via_throws_while_incomplete);

int nci = 0;
constexpr int ci = 5;
const int cc = 7;
static int snc = 1;
consteval bool extract_ref_throws(info r) {
  try { (void)extract<int&>(r); } catch (std::meta::exception&) { return true; }
  return false;
}
static_assert(extract_ref_throws(^^nci) && extract_ref_throws(^^snc));
static_assert(&extract<int&>(reflect_object(nci)) == &nci);
static_assert(extract<const int&>(^^ci) == 5);
static_assert(extract<const int&>(^^cc) == 7);
consteval int local() { int l = 3; (void)l; return extract<int&>(^^l); }
static_assert(local() == 3);

using IA = int;
void ta(IA const c, IA d, long e);
static_assert(parameters_of(type_of(^^ta))[0] == ^^int);
static_assert(parameters_of(type_of(^^ta))[1] == ^^int);
static_assert(parameters_of(type_of(^^ta))[2] == ^^long);
static_assert(type_of(parameters_of(^^ta)[1]) == ^^int);

// parameters_of of a parameter reflection throws; variable_of of a parameter
// outside its function's scope (including from a nested lambda) throws.
consteval bool parameters_of_throws(info r) {
  try { (void)parameters_of(r); } catch (std::meta::exception&) { return true; }
  return false;
}
void fn(int);
static_assert(parameters_of_throws(parameters_of(^^fn)[0]));
consteval bool variable_of_in_lambda_throws(int) {
  auto l = [&] {
    try { (void)variable_of(parameters_of(^^fn)[0]); } catch (std::meta::exception&) { return true; }
    return false;
  };
  return l();
}
static_assert(variable_of_in_lambda_throws(1));

// parameters_of throws unless given a function or function type.
template <class> struct TT {};
template <class> void ft();
static_assert(parameters_of_throws(info{}));
static_assert(parameters_of_throws(^^::));
static_assert(parameters_of_throws(^^ft));
static_assert(parameters_of_throws(^^TT));
static_assert(parameters_of_throws(reflect_constant(1)));
static_assert(parameters_of_throws(^^int));
consteval bool variable_of_other_function_throws() {
  try { (void)variable_of(parameters_of(^^fn)[0]); } catch (std::meta::exception&) { return true; }
  return false;
}
static_assert(variable_of_other_function_throws());

// size_of / alignment_of / bit_size_of throw for a function type, whose sizeof
// is ill-formed (#195).
consteval bool size_query_throws(info r) {
  bool a = false, b = false, c = false;
  try { (void)size_of(r); } catch (std::meta::exception&) { a = true; }
  try { (void)alignment_of(r); } catch (std::meta::exception&) { b = true; }
  try { (void)bit_size_of(r); } catch (std::meta::exception&) { c = true; }
  return a && b && c;
}
static_assert(size_query_throws(^^void()) && size_query_throws(^^void(int)));
static_assert(size_of(^^int) == sizeof(int) && alignment_of(^^long) == alignof(long) &&
              bit_size_of(^^char) == 8);

int main(int, char**) { return 0; }
