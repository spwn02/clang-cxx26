// RUN: clang-check -syntax-tree-dump %s -- -std=c++2d -I %S/../Reflection/Inputs | FileCheck %s

// The helper builtins (str_lit, tokenize, report_tokens) keep synthesized
// calls that read a user-defined string argument, sharing its nodes; and what
// tokenize makes is located at the call. Neither is written in the source.
#include "token-sequence.h"

namespace std::meta {
using info = decltype(^^::);
template <class... Ts> consteval auto str_lit(Ts const &...) -> token_sequence;
template <class... Ts> consteval auto tokenize(Ts const &...) -> token_sequence;
template <class M> consteval auto report_tokens(M const &, token_sequence) -> void;
consteval auto queue_injection(token_sequence) -> void;
} // namespace std::meta
using std::meta::queue_injection;

struct string_view {
  char const *p_;
  unsigned long len_;
  constexpr string_view(char const *p) : p_(p), len_(__builtin_strlen(p)) {}
  constexpr auto data() const -> char const * { return p_; }
  constexpr auto size() const -> unsigned long { return len_; }
};

static_assert(std::meta::str_lit(string_view("world")) == ^^{ "world" });

consteval std::meta::token_sequence make() {
  string_view type = "int";
  string_view name = "z";
  return std::meta::tokenize(type, " ", name, " = 200;");
}
struct S {
  consteval { queue_injection(make()); }
};
consteval void report() {
  string_view sv("from string_view");
  std::meta::report_tokens(sv, ^^{ int x; });
}
// CHECK: TranslationUnit
// CHECK: 'S'
