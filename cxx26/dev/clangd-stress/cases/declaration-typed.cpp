#include "token-sequence.h"
using std::meta::token_sequence;
__macro declare(int x) { return ^^{ int generated = \(x); }; }
declare!(1);
int f() { return generated; }
