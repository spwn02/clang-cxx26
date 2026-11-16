#include "role_header.h"

__macro highlight_as_type(token_sequence ts) {
  (void)meta_x::__role(
      ts[0], meta_x::__semantic_token_kind::type,
      static_cast<unsigned>(meta_x::__semantic_token_modifier::user_defined));
  return ^^{ \(ts) };
}

int target = 0;
int use_target = highlight_as_type!(target);

__macro warning_note_roles(token_sequence ts) {
  (void)meta_x::__warning(ts[0], "warning keeps its note");
  (void)meta_x::__role(ts[0], meta_x::__semantic_token_kind::type);
  (void)meta_x::__note(ts[1], "note remains attached");
  return ^^{ 0 };
}
int diagnostic_result = warning_note_roles!(warning_token note_token);

constexpr auto shared_tokens = ^^{ shared_name };
__macro shared_as_type() {
  (void)meta_x::__role(shared_tokens[0],
                       meta_x::__semantic_token_kind::type);
  return ^^{ 0 };
}
__macro shared_as_variable() {
  (void)meta_x::__role(shared_tokens[0],
                       meta_x::__semantic_token_kind::variable);
  return ^^{ 0 };
}
static_assert(shared_as_type!() == 0);
static_assert(shared_as_variable!() == 0);

__macro failed_role(token_sequence ts) {
  (void)meta_x::__role(ts[0], meta_x::__semantic_token_kind::type);
  (void)meta_x::__error(ts[0], "role failure");
  return ^^{ 1 + };
}
int failed = failed_role!(failed_token);
