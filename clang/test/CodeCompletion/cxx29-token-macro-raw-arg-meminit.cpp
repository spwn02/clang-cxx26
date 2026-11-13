// A completion inside the raw argument of a mem-initializer macro: the injected
// token stream is left behind when completion cuts parsing short unless the
// parser drains it (it read a stale token location and aborted intermittently).
// RUN: %clang_cc1 -std=c++29 -I %S/../Reflection/Inputs -fsyntax-only -code-completion-at=%s:14:37 %s -o - | FileCheck %s
// RUN: %python -c 'import subprocess,sys; args=sys.argv[1:]; [subprocess.run(args, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=True) for _ in range(30)]' %clang_cc1 -std=c++29 -I %S/../Reflection/Inputs -fsyntax-only -code-completion-at=%s:14:37 %s -o -

#include "token-sequence.h"
using std::meta::token_sequence;

__macro init_member(token_sequence expr) { return ^^{ member(\(expr)) }; }
int partial_value;
struct WithMember {
  int member;
  WithMember() : init_member!(partial_value) {}
};
// CHECK: COMPLETION: partial_value : [#int#]partial_value
