#include "token-sequence.h"

// Expression macros defined in a header the importer uses.
__macro inc(int x) { return ^^{ (\(x) + 1) }; }
__macro keep(std::meta::token_sequence t) { return ^^{ (\(t)) }; }

// A literal that names its own template parameter: instantiated only by the
// importer, so its parameter bindings are what make that work.
template <class T> consteval std::meta::token_sequence declare() {
  return ^^{ T v; };
}

// A dependent invocation, instantiated after the import.
template <class T> constexpr T addone(T t) { return inc!(t); }
constexpr int one_more(int n) { return inc!(n); }

// An inline function with a literal: the same one in two translation units
// must be one function (the ODR hash is of the tokens).
inline consteval std::meta::token_sequence shared() { return ^^{ a b c }; }
