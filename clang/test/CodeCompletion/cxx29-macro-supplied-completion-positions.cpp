// RUN: %clang -std=c++29 -nostdinc++ -isystem %S/../../../libcxx/include -idirafter %S/../../../build-libcxx/include/c++/v1 -fsyntax-only -Wmacro-located-diagnostics -Rmacro-role -Xclang -code-completion-at=%s:43:24 %s -o - 2>&1 | FileCheck %s --check-prefix=DECL
// RUN: not %clang -std=c++29 -nostdinc++ -isystem %S/../../../libcxx/include -idirafter %S/../../../build-libcxx/include/c++/v1 -fsyntax-only -Wmacro-located-diagnostics -Rmacro-role -Xclang -code-completion-at=%s:46:32 %s -o - 2>&1 | FileCheck %s --check-prefix=MEM
// RUN: not %clang -std=c++29 -nostdinc++ -isystem %S/../../../libcxx/include -idirafter %S/../../../build-libcxx/include/c++/v1 -fsyntax-only -Wmacro-located-diagnostics -Rmacro-role -Xclang -code-completion-at=%s:48:29 %s -o - 2>&1 | FileCheck %s --check-prefix=DECL-FAIL
// RUN: not %clang -std=c++29 -nostdinc++ -isystem %S/../../../libcxx/include -idirafter %S/../../../build-libcxx/include/c++/v1 -fsyntax-only -Wmacro-located-diagnostics -Rmacro-role -Xclang -code-completion-at=%s:50:46 %s -o - 2>&1 | FileCheck %s --check-prefix=MEM-FAIL

#include <__meta/token_locations.h>
namespace mx = std::meta::__x;
using std::meta::token_sequence;

__macro declaration(token_sequence raw) {
  (void)mx::__warning(raw[0], "declaration warning");
  (void)mx::__note(raw[0], "declaration note");
  (void)mx::__role(raw[0], mx::__semantic_token_kind::type);
  if (mx::__is_completion(raw[1]))
    (void)mx::__complete(raw[1], "hx_decl", "decl",
                         mx::__completion_kind::class_);
  return ^^{ \(raw) };
}
__macro initializer(token_sequence raw) {
  (void)mx::__warning(raw[0], "initializer warning");
  (void)mx::__note(raw[0], "initializer note");
  (void)mx::__role(raw[0], mx::__semantic_token_kind::field);
  if (mx::__is_completion(raw[0]))
    (void)mx::__complete(raw[0], "hx_mem", "mem",
                         mx::__completion_kind::field);
  return ^^{ \(raw)(0) };
}
__macro declaration_error(token_sequence raw) {
  (void)mx::__role(raw[0], mx::__semantic_token_kind::variable);
  if (mx::__is_completion(raw[1]))
    (void)mx::__complete(raw[1], "hx_decl_leak", "leak");
  (void)mx::__error(raw[0], "declaration located failure");
  return ^^{ \(raw) };
}
__macro initializer_error(token_sequence raw) {
  (void)mx::__role(raw[0], mx::__semantic_token_kind::variable);
  if (mx::__is_completion(raw[0]))
    (void)mx::__complete(raw[0], "hx_mem_leak", "leak");
  (void)mx::__error(raw[0], "initializer located failure");
  return ^^{ \(raw)(0) };
}

declaration!(int hx_decl);
struct Holder {
  int hx_member;
  Holder() : initializer!(hx_member) {}
};
declaration_error!(int hx_bad);
struct FailedHolder {
  FailedHolder() : initializer_error!(hx_bad_mem) {}
};

// DECL: warning: declaration warning
// DECL: note: declaration note
// DECL: remark: macro role kind=13
// DECL: remark: macro completion kind=3 label=hx_decl insert=decl
// DECL-NOT: hx_mem
// MEM: warning: initializer warning
// MEM: note: initializer note
// MEM: remark: macro role kind=6
// MEM: remark: macro completion kind=4 label=hx_mem insert=mem
// DECL-FAIL-NOT: macro role kind=0
// DECL-FAIL-NOT: macro completion kind=0 label=hx_decl_leak
// DECL-FAIL-NOT: did not produce a token sequence
// DECL-FAIL: declaration located failure
// DECL-FAIL-NOT: did not produce a token sequence
// DECL-FAIL-NOT: macro role kind=0
// DECL-FAIL-NOT: macro completion kind=0 label=hx_decl_leak
// MEM-FAIL-NOT: macro role kind=0
// MEM-FAIL-NOT: macro completion kind=0 label=hx_mem_leak
// MEM-FAIL-NOT: did not produce a token sequence
// MEM-FAIL: initializer located failure
// MEM-FAIL-NOT: did not produce a token sequence
// MEM-FAIL-NOT: macro role kind=0
// MEM-FAIL-NOT: macro completion kind=0 label=hx_mem_leak
