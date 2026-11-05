#include "token-sequence.h"

// The same inline function in every module that includes this header: the ODR
// hash must be of the tokens and the interpolated expression, not of where the
// compiler happened to keep them. (-DV2 changes a token.)
#ifdef V2
inline consteval std::meta::token_sequence odr(int n) { return ^^{ a \(n) - b }; }
#else
inline consteval std::meta::token_sequence odr(int n) { return ^^{ a \(n) + b }; }
#endif
