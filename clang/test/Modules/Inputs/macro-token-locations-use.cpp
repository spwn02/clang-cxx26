#include <__meta/token_locations.h>
import MacroTokens;
using std::meta::token_sequence;
namespace mx = std::meta::__x;

#ifdef TEST_ROLES
constexpr token_sequence current_role_tokens = ^^{ current_role };
__macro role_from_sequence() {
  (void)mx::__role(imported_role_tokens[0], mx::__semantic_token_kind::type);
  return ^^{ 1 };
}
__macro role_from_current_sequence() {
  (void)mx::__role(current_role_tokens[0], mx::__semantic_token_kind::type);
  return ^^{ 1 };
}
int imported_role_use = role_from_sequence!();
int current_role_use = role_from_current_sequence!();
// Imported module tokens do not produce a clangd role remark. The ordinary
// quote in this importing file still does.
// ROLE-COUNT-1: {{.*}}macro-token-locations-use.cpp:7:52: remark: macro role kind=13 modifiers=0 expansion=2
#else
__macro diagnostics_from_sequence() {
  (void)mx::__error(imported_diagnostic_tokens[0], "module imported error",
                    mx::__replace(imported_diagnostic_tokens[0],
                                  imported_diagnostic_tokens[0], "fixed"));
  (void)mx::__warning(imported_diagnostic_tokens[1], "module imported warning");
  return ^^{ 1 };
}
int imported_diagnostics = diagnostics_from_sequence!();
// DIAG: {{.*}}macro-token-locations.cppm:9:3: error: module imported error
// DIAG: fix-it:"{{.*}}macro-token-locations.cppm":{9:3-9:17}:"fixed"
// DIAG: {{.*}}macro-token-locations.cppm:10:3: warning: module imported warning
#endif
