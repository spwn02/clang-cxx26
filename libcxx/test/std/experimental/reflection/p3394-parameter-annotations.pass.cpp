//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// ADDITIONAL_COMPILE_FLAGS: -freflection-latest -fparameter-reflection

#include <meta>

using namespace std::meta;

namespace direct {
struct Inject {
  int slot;

  constexpr auto operator==(const Inject &) const -> bool = default;
};

void configure(int count [[= Inject{7}]], long timeout [[= Inject{11}]]);

constexpr auto count = parameters_of(^^configure)[0];
constexpr auto timeout = parameters_of(^^configure)[1];

static_assert(extract<Inject>(annotations_of(count)[0]).slot == 7);
static_assert(extract<Inject>(annotations_of(timeout)[0]).slot == 11);
static_assert(annotations_of(count, ^^Inject).size() == 1);
} // namespace direct

namespace redeclarations {
void configure(int count [[= 1]], long timeout);
void configure(int count, long timeout [[= 2]]);
void configure(int count [[= 3]], long timeout);

constexpr auto count = parameters_of(^^configure)[0];
constexpr auto timeout = parameters_of(^^configure)[1];

static_assert(annotations_of(count).size() == 2);
static_assert(extract<int>(annotations_of(count)[0]) == 3);
static_assert(extract<int>(annotations_of(count)[1]) == 1);
static_assert(extract<int>(annotations_of(timeout)[0]) == 2);
} // namespace redeclarations

namespace templates {
template <class T>
void configure(T value [[= 42]]);

constexpr auto value = parameters_of(^^configure<int>)[0];

static_assert(type_of(value) == ^^int);
static_assert(extract<int>(annotations_of(value)[0]) == 42);
} // namespace templates

int main() {}
