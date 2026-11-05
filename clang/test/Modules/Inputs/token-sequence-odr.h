#include "token-sequence.h"

// The same inline function in every module that includes this header: the
// ODR hash must be of the tokens and the interpolated expression, not of
// where the compiler happened to keep them.
inline consteval std::meta::token_sequence odr(int n) {
  return ^^{ a \(n) + b };
}
template <class T> consteval std::meta::token_sequence odr_t() { return ^^{ T x ; }; }
inline consteval auto odr_use() { return odr_t<int>(); }
