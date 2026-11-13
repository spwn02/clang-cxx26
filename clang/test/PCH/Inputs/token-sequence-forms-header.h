#include "token-sequence.h"

__macro inc(int x) { return ^^{ (\(x) + 1) }; }
__macro declare_it(std::meta::token_sequence name) {
  return ^^{ int \(name) = 1; };
}
struct Holder {
  int v;
  __macro bump(this Holder &self, int n) { return ^^{ (\(self).v += \(n)) }; }
};

constexpr int expression_form(int k) { return inc!(k); }
inline int member_form(Holder h) { return h.bump!(1); }
inline void statement_forms(Holder h, int k) {
  inc!(k);
  h.bump!(2);
}
template <class T> constexpr T dependent_form(T t) { return inc!(t); }
constexpr int instantiated() { return dependent_form(1); }
declare_it!(declared);

// A dependent macro lookup defers declaration, statement and mem-initializer
// arguments until the importer instantiates each template.
struct DeferredMacros {
  static __macro declare(std::meta::token_sequence name) {
    return ^^{ int \(name) = 1; };
  }
  static __macro statement(int n) { return ^^{ (void)\(n); }; }
};
template <class H> struct DeferredDeclaration { H::declare!(added); };
template <class H> constexpr int deferred_statement(int n) {
  H::statement!(n);
  return n;
}
