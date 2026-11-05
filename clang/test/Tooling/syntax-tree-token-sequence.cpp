// RUN: clang-check -syntax-tree-dump %s -- -std=c++2d | FileCheck %s

// The syntax tree builder asserts on a defaulted constructor (also in C++17),
// so the class is declared without one.
namespace std::meta {
class token_sequence {
public:
  constexpr token_sequence() noexcept {}
};
} // namespace std::meta

constexpr auto t = ^^{ a + b };
// CHECK: SimpleDeclaration
// CHECK: |-'constexpr'
// CHECK: |-'auto'
// CHECK: 't'
// CHECK: UnknownExpression
// CHECK: |-'^^'
// CHECK: |-'{'
// CHECK: |-'a'
// CHECK: |-'+'
// CHECK: |-'b'
// CHECK: `-'}'
