// RUN: clang-check -syntax-tree-dump %s -- -std=c++26 | FileCheck %s

// Declarations the syntax-tree builder used to assert on: an abbreviated
// function template, an explicit object parameter, an attribute on a later
// parameter, a requires-expression.
void abbreviated(auto &&t) {}
struct W {
  int get(this auto &self) { return 1; }
};
void attribute(int v, [[maybe_unused]] int w) {}
struct S {
  int x;
};
static_assert(requires(S s) { s.x; });
template <class T> concept Has = requires(T t) { t.x; };
// CHECK: 'abbreviated'
// CHECK: 'get'
// CHECK: 'attribute'
// CHECK: UnknownExpression

// More of what libc++ headers contain: an attribute naming a qualified type,
// a member of an anonymous union named without an object, an unnamed
// parameter pack.
namespace pmr {
struct string {};
} // namespace pmr
struct [[clang::preferred_name(pmr::string)]] Named {
  union {
    int integer;
    float real;
  };
  int read() { return integer; }
};
template <class... Ts> void unnamed_pack(Ts...);
// CHECK: 'Named'
// CHECK: 'unnamed_pack'

// A declaration whose trailing return type is a decltype ends at its closing
// parenthesis (it used to end at the name), and an implicit conversion call
// is not syntax.
auto trailing(int) -> decltype(1);
template <class T> auto trailing_member(T t) -> decltype(t.x);
struct Ordering {
  operator int() const;
};
int converted(Ordering o) { return o; }
// CHECK: 'trailing'
// CHECK: 'trailing_member'
// CHECK: 'converted'

// A member of an anonymous union reached through an object, and a variable
// initialized by a builtin the compiler evaluates itself.
struct Pair {
  union {
    int first;
    float first_as_float;
  };
};
int through_object(Pair &p) { return p.first; }
constexpr unsigned long folded = __builtin_strlen("abc");

// Out-of-line members of a class template, a redeclaration that inherits a
// default argument, an alias-declaration in a function body, a statement with
// a pragma before it.
template <class T> struct Box {
  template <class U> void put(U);
  void reset(bool force = false);
};
template <class T>
template <class U>
void Box<T>::put(U) {}
template <class T> void Box<T>::reset(bool force) {}
int with_pragma(int n) {
  using Index = int;
  int sum = 0;
#pragma clang loop vectorize(enable)
  for (Index i = 0; i < n; ++i)
    sum += i;
  return sum;
}
