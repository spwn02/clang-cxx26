// RUN: clang-check -syntax-tree-dump %s -- -std=c++26 | FileCheck %s
//
// Constructs of the standard library headers that syntax trees must build.

using info = decltype(^^int);
template <class T> constexpr unsigned long tv = 1;
template <class T> struct PF { PF(int); };
template <unsigned long N> struct Op {};
struct S { int m; };
struct Ptr { S *p; S *operator->() const; };
struct Fn { int operator()(int, int = 0, int = 1) const; };

// An out-of-line static data member of a class template with a dependent type.
template <class C> struct Str { typedef unsigned size_type; static const size_type npos = -1; };
template <class C> const typename Str<C>::size_type Str<C>::npos;
// CHECK:      TemplateDeclaration Declaration
// CHECK:        | `-'npos'

// A declaration in the condition of a while, switch and for statement.
void conditions(const char *p) {
  while (unsigned char c = static_cast<unsigned char>(*p++))
    ;
  switch (int z = 2) {}
  for (int i = 0; int y = i; ++i) {}
}
// CHECK: WhileStatement Statement
// CHECK: SwitchStatement Statement
// CHECK: ForStatement Statement

// A member access through an overloaded operator->.
struct Holder {
  Ptr w;
  int get() const { return w->m; }
};
// CHECK:      MemberExpression
// CHECK-NEXT:   | | | |-IdExpression Object
// CHECK:      '->' AccessToken

// A trailing return type whose qualifiers are written before the type.
template <class T> using Id = T;
template <class R> auto trailing(R) -> const Id<R> * { return 0; }
auto trailing2() -> const int * { return 0; }
// CHECK: TrailingReturnType TrailingReturn

// An inheriting constructor of a dependent base whose arguments hold
// expressions.
template <class F, class B> struct Inh : PF<Op<tv<B>>> {
  using PF<Op<tv<B>>>::PF;
};
// CHECK: UsingDeclaration

// A member class template defined outside its class.
template <class T> struct Outer { template <bool> class It; };
template <class T>
template <bool C>
class Outer<T>::It { int x; };
// CHECK: TemplateDeclaration Declaration
// CHECK: TemplateDeclaration Declaration

// A call of a function object with default arguments.
int callDefault(Fn f) { return f(1); }

// The same dependent splice written in two places, and a dependent member
// splice.
template <info R> int splice1() { return 1 + (typename [:R:]{}); }
template <info R> int splice2() { return 2 + (typename [:R:]{}); }
template <info R, info M> int memberSplice(S s) { return s.[:M:]; }

// An explicit instantiation of a specialization that was instantiated
// implicitly before.
template <class C> class Basic { int x; };
using BasicChar = Basic<char>;
extern template class Basic<char>;
template class Basic<int>;
// CHECK: ExplicitTemplateInstantiation
// CHECK: ExplicitTemplateInstantiation

// The declarators of one declaration share the arguments of the type.
template <int N> struct Big { void z(); };
template <int Cap> bool shared() { Big<Cap> a, b; a.z(); b.z(); return true; }

// An alias of a pack-indexed type, and the builtin templates.
template <class... T> using Last = T...[sizeof...(T) - 1];
template <class... T> void usePack(T... a) { Last<T...> x = a...[0]; }
template <class T, T... I> struct IntSeq {};
template <class T, T N> using MakeSeq = __make_integer_seq<IntSeq, T, N>;
// CHECK: TypeAliasDeclaration
