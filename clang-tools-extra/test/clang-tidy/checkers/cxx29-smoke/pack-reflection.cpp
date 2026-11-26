// RUN: clang-tidy -checks='-*,modernize-*,readability-*,performance-*' %s -- -std=c++29 2>&1 | FileCheck --allow-empty %s
// RUN: cp %s %t.cpp
// RUN: clang-tidy -checks='-*,modernize-*,readability-*,performance-*' -fix %t.cpp -- -std=c++29
// RUN: clang++ -std=c++29 -fsyntax-only %t.cpp
// CHECK-NOT: warning:
// CHECK-NOT: Assertion
// CHECK-NOT: Stack dump
// NOLINTBEGIN(readability-identifier-naming)

using info = decltype(^^int);
consteval bool is_class(info /*subject*/) { return false; } // NOLINT(modernize-use-trailing-return-type)
static_assert(^^int != ^^long);
static_assert(!is_class(^^int));
using selected = [: ^^int :];
static_assert(sizeof(selected) == sizeof(int));

template <template <class> class... TT> struct Templates {
  template <class T> using first = TT...[0]<T>;
};
template <class> struct Box {};
static_assert(__is_same(Templates<Box>::first<int>, Box<int>));

template <class T> concept Any = true;
template <template <class> concept... CC> struct Constrained {
  template <CC...[0] T> struct First {};
};
static_assert(requires { typename Constrained<Any>::First<int>; });

struct Base { int base; };
struct Object : Base { int member; };
constexpr Object object{.base = 1, .member = 2};
static_assert(object.base == 1);

static_assert(requires { { 1 } noexcept(true); });

namespace Names {
inline namespace V1 { struct Name {}; }
using V1::Name;
}
static_assert(^^Names::Name == ^^Names::V1::Name);

void *operator new(unsigned long); // NOLINT(modernize-use-trailing-return-type)
void operator delete(void *) noexcept;



// NOLINTEND(readability-identifier-naming)
