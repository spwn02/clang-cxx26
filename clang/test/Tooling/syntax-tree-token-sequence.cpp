// RUN: clang-check -syntax-tree-dump %s -- -std=c++2d | FileCheck %s

namespace std::meta {
class token_sequence {
public:
  constexpr token_sequence() noexcept = default;
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
