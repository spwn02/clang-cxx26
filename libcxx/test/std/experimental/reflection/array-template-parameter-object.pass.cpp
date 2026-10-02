//===----------------------------------------------------------------------===//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20 || c++23
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

#include <meta>
#include <array>
#include <vector>
#include <cassert>
#include <type_traits>

namespace meta = std::meta;
constexpr std::size_t dims[] = {3};
constexpr int raw[] = {1, 2, 3};
constexpr std::array<int, 3> arr = {1, 2, 3};
constexpr auto r = meta::__reflect_array_object(dims, 1, raw, 3);
constexpr auto a = meta::__reflect_array_object(dims, 1, arr.data(), 3);
constexpr auto v = [] consteval {
  std::vector<int> values = {1, 2, 3};
  return meta::__reflect_array_object(dims, 1, values.data(), values.size());
}();
static_assert(meta::is_object(r));
static_assert(!meta::is_variable(r));
static_assert(meta::type_of(r) == ^^const int[3]);
static_assert(r == a && a == v);
constexpr auto& ref = meta::extract<const int(&)[3]>(r);
static_assert(&ref == &meta::extract<const int(&)[3]>(a));
static_assert(&ref == &meta::extract<const int(&)[3]>(v));
static_assert(ref[0] == 1 && ref[2] == 3);
constexpr auto ptr = meta::extract<const int*>(r);
static_assert(ptr == ref && ptr[1] == 2);
constexpr int different[] = {1, 2, 4};
constexpr auto other = meta::__reflect_array_object(dims, 1, different, 3);
static_assert(other != r);
static_assert(&meta::extract<const int(&)[3]>(other) != &ref);

// Array sugar and prequalified leaf types must not produce separate TPOs.
using I = int;
constexpr auto sugar = meta::__reflect_array_object<I>(dims, 1, raw, 3);
constexpr auto qualified = meta::__reflect_array_object<const int>(dims, 1, raw, 3);
static_assert(sugar == r && qualified == r);

template <auto&> struct W {};
template <const int(&)[3]> struct TypedW {};
static_assert(std::is_same_v<W<meta::extract<const int(&)[3]>(r)>,
                             W<meta::extract<const int(&)[3]>(v)>>);
static_assert(std::is_same_v<TypedW<meta::extract<const int(&)[3]>(r)>,
                             TypedW<meta::extract<const int(&)[3]>(a)>>);

constexpr std::size_t ndims[] = {2, 3};
constexpr int flat[] = {1, 2, 3, 4, 5, 6};
constexpr auto nested = meta::__reflect_array_object(ndims, 2, flat, 6);
static_assert(meta::is_object(nested));
static_assert(meta::type_of(nested) == ^^const int[2][3]);
static_assert(meta::extract<const int(&)[2][3]>(nested)[1][2] == 6);
static_assert(meta::extract<const int(*)[3]>(nested)[1][0] == 4);

struct S { int x; char c; };
constexpr S structs[] = {{1, 'a'}, {2, 'b'}, {3, 'c'}};
constexpr auto classes = meta::__reflect_array_object(dims, 1, structs, 3);
static_assert(meta::is_object(classes));
static_assert(meta::type_of(classes) == ^^const S[3]);
static_assert(meta::extract<const S(&)[3]>(classes)[1].c == 'b');
static_assert(meta::extract<const S*>(classes)[2].x == 3);
constexpr auto class_copy = [] consteval {
  std::vector<S> values = {{1, 'a'}, {2, 'b'}, {3, 'c'}};
  return meta::__reflect_array_object(dims, 1, values.data(), 3);
}();
static_assert(classes == class_copy);

constexpr char chars[] = {'a', 'b', '\0'};
constexpr auto text = meta::__reflect_array_object(dims, 1, chars, 3);
static_assert(meta::is_object(text));
static_assert(meta::type_of(text) == ^^const char[3]);
static_assert(meta::extract<const char*>(text)[1] == 'b');

// Malformed shapes and empty buffers are rejected until the library supplies
// the draft's distinct const array<T, 0> TPO path.
static_assert([] consteval {
  try {
    (void)meta::__reflect_array_object(dims, 1, raw, 2);
  } catch (const meta::exception&) { return true; }
  return false;
}());
static_assert([] consteval {
  constexpr std::size_t zero[] = {0};
  try {
    (void)meta::__reflect_array_object(zero, 1, raw, 0);
  } catch (const meta::exception&) { return true; }
  return false;
}());
static_assert([] consteval {
  try {
    (void)meta::extract<int*>(r); // Cannot discard element const.
  } catch (const meta::exception&) { return true; }
  return false;
}());

// Valid pointer leaves preserve their value. A pointer into a local object
// remains invalid as a template argument.
constexpr const int* pointers[] = {raw, raw + 1, raw + 2};
constexpr auto pointer_array = meta::__reflect_array_object(dims, 1, pointers, 3);
static_assert(meta::extract<const int* const(&)[3]>(pointer_array)[1] == raw + 1);
struct Pointer { const int* p; };
constexpr Pointer pointer_members[] = {{raw}, {raw + 1}, {raw + 2}};
constexpr auto pointer_classes = meta::__reflect_array_object(dims, 1, pointer_members, 3);
static_assert(meta::extract<const Pointer*>(pointer_classes)[2].p == raw + 2);
static_assert([] consteval {
  int local = 42;
  const int* values[] = {&local, &local, &local};
  try {
    (void)meta::__reflect_array_object(dims, 1, values, 3);
  } catch (const meta::exception&) { return true; }
  return false;
}());

// Nested arrays: equal values from a raw 2-D array and a std::array of
// std::array share one object.
namespace nested_identity {
constexpr std::size_t nested_dims[] = {2, 3};
constexpr int raw2[2][3] = {{1, 2, 3}, {4, 5, 6}};
constexpr std::array<std::array<int, 3>, 2> nested = {{{1, 2, 3}, {4, 5, 6}}};
constexpr auto from_raw = [] consteval {
  std::vector<int> v;
  for (auto& row : raw2) for (int x : row) v.push_back(x);
  return meta::__reflect_array_object(nested_dims, 2, v.data(), v.size());
}();
constexpr auto from_std = [] consteval {
  std::vector<int> v;
  for (auto& row : nested) for (int x : row) v.push_back(x);
  return meta::__reflect_array_object(nested_dims, 2, v.data(), v.size());
}();
static_assert(from_raw == from_std);
static_assert(meta::type_of(from_raw) == ^^const int[2][3]);
static_assert(&meta::extract<const int(&)[2][3]>(from_raw) ==
              &meta::extract<const int(&)[2][3]>(from_std));
static_assert(meta::extract<const int(&)[2][3]>(from_raw)[1][2] == 6);
} // namespace nested_identity

int main(int, char**) {
  // Force runtime emission and reads of scalar, nested, and class TPOs.
  volatile int index = 1;
  assert(ptr[index] == 2);
  assert(meta::extract<const int(*)[3]>(nested)[index][2] == 6);
  assert(meta::extract<const S*>(classes)[index].c == 'b');
  assert(&ref == &meta::extract<const int(&)[3]>(v));
}
