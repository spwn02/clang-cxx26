#include "token-sequence.h"
using std::meta::token_sequence;
struct S { int v; __macro set(this S& self, int n) { return ^^{ (\(self).v = \(n)) }; } };
int f() { S s{}; s.set!(1); return s.v; }
