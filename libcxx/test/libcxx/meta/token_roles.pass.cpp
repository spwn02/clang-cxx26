// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20, c++23, c++26

#include <__meta/token_locations.h>
#include <meta>

using std::meta::token_sequence;
namespace mx = std::meta::__x;

static_assert(static_cast<unsigned>(mx::__semantic_token_kind::variable) == 0);
static_assert(static_cast<unsigned>(mx::__semantic_token_kind::label) == 22);
static_assert(static_cast<unsigned>(mx::__semantic_token_modifier::declaration) ==
              (1u << 0));
static_assert(static_cast<unsigned>(mx::__semantic_token_modifier::global_scope) ==
              (1u << 17));

__macro mark_one(token_sequence ts) {
  (void)mx::__role(ts[0], mx::__semantic_token_kind::type);
  return ^^{ 42 };
}

__macro mark_range(token_sequence ts) {
  (void)mx::__role(ts[0], ts[1], mx::__semantic_token_kind::modifier,
                   static_cast<unsigned>(
                       mx::__semantic_token_modifier::user_defined));
  return ^^{ 7 };
}

static_assert(mark_one!(token) == 42);
static_assert(mark_range!(first second) == 7);

int main() {}
