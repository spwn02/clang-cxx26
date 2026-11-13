// expected-no-diagnostics
#include "token-sequence.h"
import M;

static_assert(exported == ^^{ x . y ( 1 , "s" ) });
static_assert(!(exported == ^^{ x . y ( 2 , "s" ) }));
static_assert(nested == ^^{ outer + ^^{ inner * \(2) } + \(42) + "text"; });
static_assert(make() == ^^{ return 42 ; ; });
