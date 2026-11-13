#include "token-sequence.h"
using std::meta::token_sequence;
__macro plus(token_sequence x) { return ^^{ (\(x) + 1) }; }
template<class T> T f(T a) { return plus!(a); }
int use() { return f(1); }
