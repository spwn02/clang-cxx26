//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03 || c++11 || c++14 || c++17 || c++20 || c++23
// ADDITIONAL_COMPILE_FLAGS: -freflection-latest

// Regression for #185: exceptions must be catchable during constant evaluation.
#include <meta>
#include <array>
#include <vector>
#include <tuple>
#include <variant>
using namespace std::meta;
template<class Probe> consteval bool throws_exception(Probe probe) {
  try { probe(); } catch (const exception&) { return true; }
  return false;
}
template<class Probe> consteval bool returns_normally(Probe probe) {
  try { probe(); } catch (const exception&) { return false; }
  return true;
}
#define THROWS(...) static_assert(throws_exception([] consteval { (void)(__VA_ARGS__); }))
#define NORMAL(...) static_assert(returns_normally([] consteval { (void)(__VA_ARGS__); }))
namespace NS {}
struct Incomplete;
struct C { int member; unsigned bits : 3; static int sm; void mf(); };
struct Other { int member; };
struct Op { bool operator==(const Op&) const; };
struct Abstract { virtual void f() = 0; };
struct VirtualAbstract : virtual Abstract {};
enum class E { a };
enum class Opaque : int;
int global;
int& ref = global;
constexpr int value = 42;
constexpr int array[] = {1, 2};
void fn([[maybe_unused]] int parameter) {}
void noargs() {}
template<class> struct Box {};
template<class T> requires (sizeof(T) == 1) struct Small {};
template<class T> auto undeduced();
[[maybe_unused]] constexpr auto unchecked = access_context::unchecked();
[[maybe_unused]] constexpr auto closure = [] {};
struct Ptr { const char* p; };

struct ThrowBegin {
  using value_type = int;
  constexpr int* begin() { if consteval { throw exception("range", ^^int); } else { return nullptr; } }
  constexpr int* end() { return nullptr; }
};
struct ThrowCopy {
  bool fail;
  constexpr ThrowCopy(bool b) : fail(b) {}
  constexpr ThrowCopy(const ThrowCopy& other) : fail(other.fail) {
    if consteval { if (fail) throw exception("copy", ^^int); }
  }
};

extern int& unknown_reference;
thread_local int thread_object;
auto pending();
[[maybe_unused]] constexpr const int* const_pointer = &value;
struct ConstMember { const int member; };
void noexcept_fn() noexcept {}

// Clause 32:
// \throws
// \tcode{meta::exception} unless
// \begin{itemize}
// \item
//   \tcode{r} represents a variable or object of type \tcode{U},
// \item
//   \tcode{is_convertible_v<remove_reference_t<U>(*)[],\brk{} remove_reference_t<\brk{}T>(\brk{}*)[]>}
//   is \tcode{true},\newline and
//   \begin{note}
//   The intent is to allow only qualification conversion from \tcode{U} to \tcode{T}.
//   \end{note}
// \item
//   If \tcode{r} represents a variable,
//   then either that variable is usable in constant expressions
//   or its lifetime began within the core constant expression currently under evaluation.
// \end{itemize}
THROWS(extract<const int&>(info{}));
THROWS(extract<const int&>(^^int));
THROWS(extract<const int&>(reflect_constant(42)));
THROWS(extract<int&>(^^value));
THROWS(extract<const float&>(^^value));
THROWS(extract<int&>(^^global));
NORMAL(extract<const int&>(^^value));
THROWS(extract<int&>(^^unknown_reference));
static_assert(returns_normally([] consteval { int local = 1; (void)extract<int&>(^^local); }));

// Clause 33:
// \throws
// \tcode{meta::exception} unless
// \begin{itemize}
// \item
//   \tcode{r} represents a non-static data member with type \tcode{X},
//   that is not a bit-field,
//   that is a direct member of class \tcode{C},
//   \tcode{T} and \tcode{X C::*} are similar types\iref{conv.qual}, and
//   \tcode{is_convertible_v<X C::*, T>} is \tcode{true};
// \item
//   \tcode{r} represents an implicit object member function
//   with type \tcode{F} or \tcode{F noexcept}
//   that is a direct member of a class \tcode{C},
//   and \tcode{T} is \tcode{F C::*}; or
// \item
//   \tcode{r} represents a non-member function,
//   static member function, or
//   explicit object member function
//   of function type \tcode{F} or \tcode{F noexcept},
//   and \tcode{T} is \tcode{F*}.
// \end{itemize}
THROWS(extract<int C::*>(^^C::bits));
THROWS(extract<float C::*>(^^C::member));
THROWS(extract<int Other::*>(^^C::member));
THROWS(extract<int (C::*)()>(^^C::mf));
THROWS(extract<int (*)()>(^^noargs));
NORMAL(extract<int C::*>(^^C::member));
NORMAL(extract<void (C::*)()>(^^C::mf));
NORMAL(extract<void (*)()>(^^noargs));
THROWS(extract<int C::*>(^^C::sm));
THROWS(extract<int ConstMember::*>(^^ConstMember::member));
THROWS(extract<void (*)() noexcept>(^^noargs));
NORMAL(extract<void (*)()>(^^noexcept_fn));

// Clause 34:
// \throws
// \tcode{meta::exception} unless
// \begin{itemize}
// \item
//   \tcode{U} is a pointer type,
//   \tcode{T} and \tcode{U} are either similar\iref{conv.qual}
//   or both function pointer types, and
//   \tcode{is_convertible_v<U, T>} is \tcode{true},
// \item
//   \tcode{U} is not a pointer type
//   and the cv-unqualified types of \tcode{T} and \tcode{U} are the same,
// \item
//   \tcode{U} is an array type,
//   \tcode{T} is a pointer type,
//   \tcode{remove_extent_t<U>*} and \tcode{T} are similar types, and
//   the value \tcode{r} represents is convertible to \tcode{T}, or
// \item
//   \tcode{U} is a closure type,
//   \tcode{T} is a function pointer type, and
//   the value that \tcode{r} represents is convertible to \tcode{T}.
// \end{itemize}
THROWS(extract<float>(reflect_constant(1)));
THROWS(extract<float*>(reflect_constant(static_cast<int*>(nullptr))));
THROWS(extract<int*>(reflect_constant(static_cast<const int*>(nullptr))));
THROWS(extract<float*>(^^array));
THROWS(extract<int (*)()>(reflect_constant([] { return 1.0; })));
NORMAL(extract<int>(reflect_constant(1)));
NORMAL(extract<const int*>(reflect_constant(static_cast<int*>(nullptr))));
NORMAL(extract<const int*>(^^array));
NORMAL(extract<int (*)()>(reflect_constant([] { return 1; })));
THROWS(extract<void (*)() noexcept>(reflect_constant(static_cast<void(*)()>(noargs))));
NORMAL(extract<void (*)()>(reflect_constant(static_cast<void(*)() noexcept>(noexcept_fn))));

// Clause 35:
// \throws
// \tcode{meta::exception} unless
// \tcode{templ} represents a template,
// and every reflection in \tcode{arguments} represents a construct
// usable as a template argument\iref{temp.arg}.
THROWS(can_substitute(info{}, {^^int}));
THROWS(can_substitute(^^Box, {^^NS}));
NORMAL(can_substitute(^^Box, {^^int}));
NORMAL(can_substitute(^^Small, {^^int}));
THROWS(can_substitute(^^Box, {info{}}));
THROWS(can_substitute(^^Box, {^^C::member}));
static_assert(!can_substitute(^^undeduced, {^^int}));

// Clause 36:
// \throws
// \tcode{meta::exception} unless
// \tcode{can_substitute(templ, arguments)} is \tcode{true}.
THROWS(substitute(info{}, {^^int}));
THROWS(substitute(^^Box, {^^NS}));
NORMAL(substitute(^^Box, {^^int}));
THROWS(substitute(^^Box, {}));
THROWS(substitute(^^Small, {^^int}));
THROWS(substitute(^^undeduced, {^^int}));
THROWS(substitute(^^Box, {info{}}));
THROWS(substitute(^^Box, {^^C::member}));

#undef THROWS
#undef NORMAL
int main(int, char**) {}
