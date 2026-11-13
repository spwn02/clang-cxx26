#include "token-sequence.h"
using std::meta::token_sequence;
__macro bump(token_sequence x) { return ^^{ if (\(x)) ++g; }; }
void f(bool a) { bump!(a); }
