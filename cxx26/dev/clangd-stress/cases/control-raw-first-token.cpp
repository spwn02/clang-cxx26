#include "token-sequence.h"
using std::meta::token_sequence;
__macro use(token_sequence expr) { return ^^{ \(expr) }; }
int partial_value;
int first() { return use!(/*CURSOR*/ + 1); }
