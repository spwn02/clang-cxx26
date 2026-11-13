#include "token-sequence.h"
using std::meta::token_sequence;
struct C { int x; __macro init(this C&, token_sequence v) { return v; } C() : init!(x(3)) {} };
