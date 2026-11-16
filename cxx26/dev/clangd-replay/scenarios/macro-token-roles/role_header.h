#pragma once
#include <__meta/token_locations.h>

namespace meta_x = std::meta::__x;
using std::meta::token_sequence;

__macro role_in_header(token_sequence ts) {
  (void)meta_x::__role(ts[0], meta_x::__semantic_token_kind::type);
  return ^^{ 0 };
}
inline int header_role_result = role_in_header!(header_token);
