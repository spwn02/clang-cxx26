#include "token-sequence.h"
using std::meta::token_sequence;
__macro plus(token_sequence x) { return ^^{ (\(x) + 1) }; }
int f(int a) { return plus!(a + 2); }
