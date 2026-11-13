#include "token-sequence.h"
using std::meta::token_sequence;
struct C { int x; __macro init(this C&, int v) { return ^^{ x(\(v)) }; } C() : init!(3) {} };
