// RUN: %clang -std=c++29 -nostdinc++ -isystem %S/../../../libcxx/include -idirafter %S/../../../build-libcxx/include/c++/v1 -fsyntax-only -fcxx-exceptions -Wmacro-located-diagnostics -Xclang -verify %s
// RUN: not %clang -std=c++29 -nostdinc++ -isystem %S/../../../libcxx/include -idirafter %S/../../../build-libcxx/include/c++/v1 -fsyntax-only -fcxx-exceptions -Wmacro-located-diagnostics -Rmacro-role %s 2>&1 | FileCheck %s --check-prefix=ROLE
// RUN: not %clang -std=c++29 -nostdinc++ -isystem %S/../../../libcxx/include -idirafter %S/../../../build-libcxx/include/c++/v1 -fsyntax-only -fcxx-exceptions -Wmacro-located-diagnostics %s 2>&1 | FileCheck %s --check-prefix=QUIET

#include <__meta/token_locations.h>

using std::meta::token_sequence;
namespace mx = std::meta::__x;

__macro roles_with_diagnostics(token_sequence ts) {
  (void)mx::__warning(ts[0], "located warning before roles");
  (void)mx::__role(ts[0], mx::__semantic_token_kind::type);
  (void)mx::__note(ts[1], "located note after warning");
  (void)mx::__role(ts[1], mx::__semantic_token_kind::modifier,
                   static_cast<unsigned>(mx::__semantic_token_modifier::user_defined));
  (void)mx::__role(ts[2], ts[4], mx::__semantic_token_kind::bracket);
  return ^^{ 1 };
}
int good = roles_with_diagnostics!(warning_token note_token range_first , range_last trailing); // expected-warning {{located warning before roles}} expected-note {{located note after warning}}

__macro failed_with_role(token_sequence ts) {
  (void)mx::__role(ts[0], mx::__semantic_token_kind::type);
  (void)mx::__error(ts[0], "failed expansion");
  return ^^{ 1 + };
}
int bad = failed_with_role!(failure_token); // expected-error {{failed expansion}}

// ROLE: warning: located warning before roles
// ROLE: note: located note after warning
// ROLE: remark: macro role kind=13 modifiers=0 expansion=
// ROLE: remark: macro role kind=19 modifiers=8192 expansion=
// ROLE-COUNT-3: remark: macro role kind=21 modifiers=0 expansion=
// ROLE-NOT: remark: macro role
// QUIET-NOT: macro role
