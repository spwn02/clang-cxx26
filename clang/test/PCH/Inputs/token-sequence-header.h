#include "token-sequence.h"

constexpr std::meta::token_sequence global_ts = ^^{ a + b * 3 };
constexpr std::meta::token_sequence empty_ts = ^^{ };

consteval std::meta::token_sequence make() {
  return ^^{ int x = 1; "text"; 'c'; 1.5f; };
}

consteval std::meta::token_sequence joined() { return make() + ^^{ ; return x; }; }

// Interpolated values become annotation tokens (a type, a template name, an
// expression): they are written with what they carry.
template <class T> struct Box {};
constexpr std::meta::token_sequence with_type = ^^{ \(^^int) v };
constexpr std::meta::token_sequence with_template = ^^{ \(^^Box)<int> b };
constexpr std::meta::token_sequence with_value = ^^{ x = \(42) };
namespace ns { constexpr int q = 5; }
constexpr std::meta::token_sequence with_namespace = ^^{ \(^^ns)::q };
