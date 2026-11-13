#include "token-sequence.h"
using std::meta::token_sequence;
__macro plus(token_sequence x) { return ^^{ (\(x) + 1) }; }
__macro identity(token_sequence x) { return x; }
int f(int a) { return plus!(identity!(a)); }
