#include "token-sequence.h"

// Dependent invocations: the arguments are parsed at instantiation, in the
// context the invocation was written in, with the template parameters in scope
// there; a member invocation may carry explicit template arguments.
struct Holder {
  int v;
  __macro bump(this Holder &self, int n) { return ^^{ (\(self).v += \(n)) }; }
  template <class U>
  __macro tm(this Holder &self, int n) { return ^^{ (\(self).v += sizeof(U) * \(n)) }; }
};
__macro plus_one(int x) { return ^^{ (\(x) + 1) }; }

template <class T> constexpr int use_member(T t) { t.bump!(1); return t.v; }
template <class T> constexpr int use_member_tmpl(T t) { t.tm!<int>(2); return t.v; }
template <class T> constexpr int use_free(T t) { return plus_one!(t.v); }
template <class T> constexpr int use_arrow(T *t) { t->bump!(3); return t->v; }

// The environment is a class template's too.
template <class T> struct Box {
  T held;
  constexpr int use() { held.bump!(sizeof(T)); return held.v; }
};
