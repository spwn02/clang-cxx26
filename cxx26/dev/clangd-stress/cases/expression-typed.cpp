#include "token-sequence.h"
using std::meta::token_sequence;
__macro plus(int x) { return ^^{ (\(x) + 1) }; }
int f() { return plus!(2); }
