#include "token-sequence.h"

// Adjacent numeric literals: the PCH writer stores their spellings one after
// the other, so the byte after "12" is the first character of "34".
constexpr std::meta::token_sequence two_numbers = ^^{ 12, 34 };
constexpr std::meta::token_sequence three_numbers = ^^{ 7, 8.5, 0x10 };
