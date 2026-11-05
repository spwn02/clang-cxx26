#include "token-sequence.h"

constexpr std::meta::token_sequence global_ts = ^^{ a + b * 3 "str" 'c' 2.5 };
constexpr std::meta::token_sequence with_value = ^^{ x = \(42) };

consteval std::meta::token_sequence make() {
  return ^^{ int x = 1; return x; } + ^^{ ; };
}

__macro inc(int x) { return ^^{ (\(x) + 1) }; }
template <class T> auto bump(T t) { return inc!(t); }

struct Rs { int m; };
constexpr std::meta::token_sequence with_refl = ^^{ x = \(^^int) + \(^^Rs) + \(^^Rs::m) };

struct HolderM {
  int v;
  __macro bump(this HolderM &self, int n) { return ^^{ (\(self).v += \(n)) }; }
};
inline int resolved_expr(int k) { return inc!(k); }
inline int resolved_member(HolderM h) { return h.bump!(1); }
inline void resolved_stmt(int k) { inc!(k); }
