#include "token-sequence.h"
using std::meta::token_sequence;
__macro declare_it(token_sequence name) { return ^^{ int \(name) = 1; }; }
declare_it!(foo);
int f() { return foo; }
