#include "token-sequence.h"
using std::meta::token_sequence;
int takes(int named) { return named; }
__macro declare(token_sequence n) { return ^^{ auto \(n) = takes(1); }; }
declare!(value);
int f() { return value; }
