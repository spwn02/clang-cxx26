#include "token-sequence.h"
using std::meta::token_sequence;
int takes(int named) { return named; }
__macro branch() { return ^^{ if (true) { takes(1); } }; }
void f() { branch!(); }
