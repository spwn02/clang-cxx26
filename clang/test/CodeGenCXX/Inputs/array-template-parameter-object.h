// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#ifndef ARRAY_TEMPLATE_PARAMETER_OBJECT_H
#define ARRAY_TEMPLATE_PARAMETER_OBJECT_H
using info = decltype(^^int);
using size_t = decltype(sizeof(0));
// Keep these IDs synchronized with libcxx/include/meta's detail enum.
template <class T>
consteval info array_object(const size_t* dims, size_t rank, const T* data,
                           size_t count) {
  return __metafunction(133, ^^T, dims, rank, data, count, ^^array_object<T>);
}
template <class T> consteval T extract(info r) {
  return __metafunction(25, ^^T, r, ^^extract<T>);
}
constexpr size_t dims[] = {3};
constexpr int values[] = {1, 2, 3};
constexpr info r = array_object(dims, 1, values, 3);
constexpr const int* pointer = extract<const int*>(r);
inline constexpr auto& reference = extract<const int(&)[3]>(r);
static_assert(__metafunction(78, r));
static_assert(__metafunction(16, r, ^^array_object<int>) == ^^const int[3]);
static_assert(pointer[2] == 3);
static_assert(&reference[0] == pointer);
template <const int(&V)[3]> struct W {
  static constexpr int value = 42;
  static int read() { return V[2]; }
};
using Specialization = W<extract<const int(&)[3]>(r)>;
constexpr size_t ndims[] = {2, 3};
constexpr int flat[] = {1, 2, 3, 4, 5, 6};
constexpr info nested = array_object(ndims, 2, flat, 6);
struct Leaf { int value; };
constexpr Leaf leaves[] = {{1}, {2}, {3}};
constexpr info classes = array_object(dims, 1, leaves, 3);
static_assert(extract<const int(&)[2][3]>(nested)[1][2] == 6);
static_assert(extract<const Leaf(&)[3]>(classes)[1].value == 2);
#endif
