#include "token-sequence.h"
using std::meta::token_sequence;
__macro init_member(token_sequence expr) { return ^^{ member(\(expr)) }; }
int partial_value;
struct WithMember {
  int member;
  WithMember() : init_member!(/*CURSOR*/partial_value) {}
};
