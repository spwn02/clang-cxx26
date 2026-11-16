// RUN: %clang -std=c++29 -nostdinc++ -isystem %S/../../../libcxx/include -idirafter %S/../../../build-libcxx/include/c++/v1 -x c++-header -Xclang -emit-pch %S/Inputs/macro-token-locations.h -o %t.pch
// RUN: not %clang -std=c++29 -nostdinc++ -isystem %S/../../../libcxx/include -idirafter %S/../../../build-libcxx/include/c++/v1 -include-pch %t.pch -fsyntax-only -Wmacro-located-diagnostics -fdiagnostics-parseable-fixits %s 2>&1 | FileCheck %s --check-prefix=DIAG
// RUN: %clang -std=c++29 -nostdinc++ -isystem %S/../../../libcxx/include -idirafter %S/../../../build-libcxx/include/c++/v1 -include-pch %t.pch -fsyntax-only -Rmacro-role -DTEST_ROLES %s 2>&1 | FileCheck %s --check-prefix=ROLE

#include <__meta/token_locations.h>
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
// ROLE: {{.*}}macro-token-locations.h:8:3: remark: macro role kind=13 modifiers=0 expansion=1
// ROLE: {{.*}}macro-token-locations.cpp:{{[0-9]+}}:{{[0-9]+}}: remark: macro role kind=13 modifiers=0 expansion=
// ROLE-NOT: remark: macro role
#else
__macro diagnostics_from_sequence() {
  (void)mx::__error(imported_diagnostic_tokens[0], "imported error",
                    mx::__replace(imported_diagnostic_tokens[0],
                                  imported_diagnostic_tokens[0], "fixed"));
  (void)mx::__warning(imported_diagnostic_tokens[1], "imported warning");
  return ^^{ 1 };
}
int imported_diagnostics = diagnostics_from_sequence!();
// DIAG: {{.*}}macro-token-locations.h:4:3: error: imported error
// The identifier's serialized token length is zero; the fix-it end is recovered from its spelling.
// DIAG: fix-it:"{{.*}}macro-token-locations.h":{4:3-4:17}:"fixed"
// DIAG: {{.*}}macro-token-locations.h:5:3: warning: imported warning
#endif
