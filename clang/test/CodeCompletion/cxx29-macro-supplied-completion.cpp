// RUN: not %clang -std=c++29 -nostdinc++ -isystem %S/../../../libcxx/include -idirafter %S/../../../build-libcxx/include/c++/v1 -fsyntax-only -Rmacro-role -Xclang -code-completion-at=%s:40:14 %s -o - 2>&1 | FileCheck %s --check-prefix=EMPTY
// RUN: not %clang -std=c++29 -nostdinc++ -isystem %S/../../../libcxx/include -idirafter %S/../../../build-libcxx/include/c++/v1 -fsyntax-only -Rmacro-role -Xclang -code-completion-at=%s:41:17 %s -o - 2>&1 | FileCheck %s --check-prefix=IDENT
// RUN: not %clang -std=c++29 -nostdinc++ -isystem %S/../../../libcxx/include -idirafter %S/../../../build-libcxx/include/c++/v1 -fsyntax-only -Rmacro-role -Xclang -code-completion-at=%s:42:17 %s -o - 2>&1 | FileCheck %s --check-prefix=MIDDLE
// RUN: %clang -std=c++29 -nostdinc++ -isystem %S/../../../libcxx/include -idirafter %S/../../../build-libcxx/include/c++/v1 -fsyntax-only -Rmacro-role -Xclang -code-completion-at=%s:43:21 %s -o -
// RUN: %clang -std=c++29 -nostdinc++ -isystem %S/../../../libcxx/include -idirafter %S/../../../build-libcxx/include/c++/v1 -fsyntax-only -Rmacro-role -Xclang -code-completion-at=%s:44:24 %s -o - 2>&1 | FileCheck %s --check-prefix=INJECT
// RUN: not %clang -std=c++29 -nostdinc++ -isystem %S/../../../libcxx/include -idirafter %S/../../../build-libcxx/include/c++/v1 -fsyntax-only -Rmacro-role -Xclang -code-completion-at=%s:45:21 %s -o - 2>&1 | FileCheck %s --check-prefix=FAIL

#include <__meta/token_locations.h>

namespace mx = std::meta::__x;
__macro html(std::meta::token_sequence raw) {
  for (std::meta::token t : raw)
    if (mx::__is_completion(t)) {
      (void)mx::__complete(t, "hx_alpha", "data-foo",
                           mx::__completion_kind::property, "attribute",
                           "HTML attribute");
      (void)mx::__complete(t, "hx_beta", "data-bar",
                           mx::__completion_kind::property, "attribute",
                           "Second attribute");
    }
  return ^^{ };
}
__macro inject(std::meta::token_sequence raw) {
  if (mx::__is_completion(raw[0]))
    (void)mx::__complete(raw[0], "hx_gamma", "gamma",
                         mx::__completion_kind::text);
  return ^^{ \(raw) };
}
__macro fail(std::meta::token_sequence raw) {
  if (mx::__is_completion(raw[0])) {
    (void)mx::__complete(raw[0], "hx_leak", "leak",
                         mx::__completion_kind::text);
    (void)mx::__error(raw[0], "failed completion expansion");
  }
  return ^^{ \(raw) };
}
int partial_value;

void test() {
  html!(<div >);
  html!(<div cla>);
  html!(<div class>);
  html!(<div title="x">);
  (void)inject!(source_token);
  (void)fail!(hx_leak);
}

// EMPTY: remark: macro completion kind=2 label=hx_alpha insert=data-foo
// EMPTY: remark: macro completion kind=2 label=hx_beta insert=data-bar
// EMPTY-NOT: COMPLETION:
// IDENT: remark: macro completion kind=2 label=hx_alpha insert=data-foo
// IDENT: remark: macro completion kind=2 label=hx_beta insert=data-bar
// IDENT-NOT: COMPLETION:
// MIDDLE: remark: macro completion kind=2 label=hx_alpha insert=data-foo
// MIDDLE: remark: macro completion kind=2 label=hx_beta insert=data-bar
// MIDDLE-NOT: COMPLETION:
// INJECT: remark: macro completion kind=0 label=hx_gamma insert=gamma
// FAIL: failed completion expansion
// FAIL-NOT: macro completion kind=0 label=hx_leak
// FAIL-NOT: COMPLETION:
