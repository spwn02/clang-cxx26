#include "token-sequence.h"
using std::meta::token_sequence;

int helper(int value) { return value; }
namespace left { int pick() { return 1; } }
namespace right { int pick() { return 2; } }

__macro call_helper() { return ^^{ helper(1) }; }
__macro choose_pick() { return ^^{ pick() }; }
__macro declare_name(token_sequence name) {
  return ^^{ int \(name) = 1; };
}
__macro template_call(token_sequence arg) {
  return ^^{ helper(\(arg)) };
}

int use_helper() { return call_helper!(); }
int left_use() { using namespace left; return choose_pick!(); }
int right_use() { using namespace right; return choose_pick!(); }
declare_name!(declared_by_quote);
template <class T> int use_template(T value) { return template_call!(value); }
int template_use() { return use_template(2); }
