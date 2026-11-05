#include "token-sequence.h"

constexpr std::meta::token_sequence global_ts = ^^{ a + b * 3 "str" 'c' 2.5 };
constexpr std::meta::token_sequence with_value = ^^{ x = \(42) };

consteval std::meta::token_sequence make() {
  return ^^{ int x = 1; return x; } + ^^{ ; };
}
