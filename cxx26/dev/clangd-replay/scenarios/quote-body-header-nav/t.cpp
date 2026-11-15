#include "token-sequence.h"
using std::meta::token_sequence;
int helper(int value) { return value; }
__macro call() { return ^^{ helper(1) }; }
int use() { return call!(); }
