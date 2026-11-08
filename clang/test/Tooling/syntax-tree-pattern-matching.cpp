// RUN: clang-check -syntax-tree-dump %s -- -std=c++2d | FileCheck %s
//
// C++29 pattern matching (P2688): shapes of match selections over an alternative
// type that the syntax tree builder has to take apart. (The unit tests cannot
// write `^^`, the annotations of their sources use `^`.)

struct Alt {
  unsigned state;
  int i;
  double d;
};

namespace std {
template <class T> struct alternative_traits;
struct alternative_info {
  decltype(^^int) info = {};
  bool empty = false;
  consteval alternative_info(decltype(^^int) i = {}, bool e = false)
      : info(i), empty(e) {}
};
} // namespace std

template <> struct std::alternative_traits<Alt> {
  static constexpr alternative_info alternatives[] = {^^int, ^^double};
  static constexpr bool has_residual_states = false;
  struct names {
    __SIZE_TYPE__ value;
    template <__SIZE_TYPE__ I> static constexpr __SIZE_TYPE__ index = I;
    constexpr operator __SIZE_TYPE__() const noexcept { return value; }
    friend constexpr bool operator==(names, names) = default;
  };
  static constexpr names index(const Alt &a) noexcept { return {a.state}; }
  template <__SIZE_TYPE__ I, class Self>
  static constexpr decltype(auto) get(Self &&a) {
    if constexpr (I == 0)
      return (a.i);
    else
      return (a.d);
  }
};

int use(int);
int use(double);

// The handler is built once per alternative by the compiler: the tree has the
// handler as it is written, and the declaration pattern has its declaration.
void projection(Alt alt) {
  match (alt) { case { auto &&payload } => use(payload); }
}
// CHECK:      MatchSelectionStatement Statement
// CHECK:      | |-AlternativePattern Pattern
// CHECK-NEXT: | | |-'{' OpenParen
// CHECK-NEXT: | | |-DeclarationPattern Pattern
// CHECK-NEXT: | | | `-SimpleDeclaration Declaration
// CHECK:      | | `-'}' CloseParen
// CHECK-NEXT: | |-'=>' ArrowToken
// CHECK-NEXT: | `-ExpressionStatement Statement
// CHECK:      |   |   `-'payload'
// CHECK-NOT:  MatchCase
// CHECK:      `-'}' CloseParen

// The variables an or-pattern introduces for its alternatives are not written.
void or_bindings(Alt alt) {
  match (alt) { case { int value } or { double value } => use(value); }
}
// CHECK:      MatchSelectionStatement Statement
// CHECK:      | | |-OrPattern Pattern
// CHECK-NEXT: | | | `-PatternAlternatives Alternatives
// CHECK-NEXT: | | |   |-AlternativePattern ListElement
// CHECK:      | | |   | | `-SimpleDeclaration Declaration
// CHECK:      | | |   |-'or' ListDelimiter
// CHECK-NEXT: | | |   `-AlternativePattern ListElement
// CHECK:      | | |     | `-SimpleDeclaration Declaration
// CHECK:      | |-'=>' ArrowToken

// A type selector and an indexed selector.
void selectors(Alt alt) {
  match (alt) {
    case { int: auto &x } => use(x);
    case { .index<1>: double y } => use(y);
  }
}
// CHECK:      MatchSelectionStatement Statement
// CHECK:      | |-MatchCase Case
// CHECK:      | | |-AlternativePattern Pattern
// CHECK:      | | | |-'{' OpenParen
// CHECK:      | | | |-DeclarationPattern Pattern
// CHECK:      | | | `-'}' CloseParen
// CHECK:      | |-MatchCase Case
// CHECK:      | | |-AlternativePattern Pattern
// CHECK:      | | | |-DeclarationPattern Pattern
// CHECK:      | | | | `-SimpleDeclaration Declaration

// A selector that is a type constraint.
template <class T> concept Integral = __is_integral(T);
void constrained_selector(Alt alt) {
  match (alt) {
    case { Integral: auto &x } => use(x);
    case { double d } => use(d);
  }
}
// CHECK:      MatchSelectionStatement Statement
// CHECK:      | |-MatchCase Case
// CHECK:      | | |-AlternativePattern Pattern
// CHECK:      | | | |-'{' OpenParen
// CHECK:      | | | |-DeclarationPattern Pattern
// CHECK-NEXT: | | | | `-SimpleDeclaration Declaration
// CHECK:      | | | `-'}' CloseParen
// CHECK:      | |-MatchCase Case

// The elements that a type pack expands to.
template <typename... Types>
void type_pack(const auto &tuple) {
  match (tuple) { case [Types...] => use(1); case _ => use(0); }
}
// CHECK:      MatchSelectionStatement Statement
// CHECK:      | |-MatchCase Case
// CHECK:      | | |-DecompositionPattern Pattern
// CHECK-NEXT: | | | |-'[' OpenParen
// CHECK-NEXT: | | | |-PatternElements Elements
// CHECK:      | | | `-']' CloseParen
// CHECK:      | |-MatchCase Case
// CHECK:      | | |-WildcardPattern Pattern
