#include "token-sequence.h"
using std::meta::token_sequence;
int takes(int named) { return named; }
__macro invoke() { return ^^{ takes(1) }; }
int f() { return invoke!(); }
