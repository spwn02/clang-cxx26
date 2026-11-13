#include "token-sequence.h"

__macro bad(int value) { return ^^{ \(value) + very_long_missing_identifier }; }
template <class T> int invoke(T value) { return bad!(value); }
__macro good(int value) { return ^^{ \(value) + 1 }; }
template <class T> int invoke_good(T value) { return good!(value); }
