#include "token-sequence.h"
using std::meta::token_sequence;
__macro bump(int x) { return ^^{ if (\(x)) ++g; }; }
void f() { bump!(1); }
