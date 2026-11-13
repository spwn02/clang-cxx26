// RUN: %clang_cc1 -std=c++29 -I %S/../Reflection/Inputs -fsyntax-only -code-completion-at=%s:17:36 %s -o - | FileCheck %s --check-prefix=EXPR
// RUN: %clang_cc1 -std=c++29 -I %S/../Reflection/Inputs -fsyntax-only -code-completion-at=%s:19:39 %s -o - | FileCheck %s --allow-empty --check-prefix=DROP
// RUN: %clang_cc1 -std=c++29 -I %S/../Reflection/Inputs -fsyntax-only -code-completion-at=%s:21:42 %s -o - | FileCheck %s --check-prefix=STMT
// RUN: %clang_cc1 -std=c++29 -I %S/../Reflection/Inputs -fsyntax-only -code-completion-at=%s:23:22 %s -o - | FileCheck %s --check-prefix=DECL
// RUN: %clang_cc1 -std=c++29 -I %S/../Reflection/Inputs -fsyntax-only -code-completion-at=%s:25:27 %s -o - | FileCheck %s --check-prefix=FIRST
// RUN: %python -c 'import subprocess,sys; args=sys.argv[1:]; [subprocess.run(args, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=True) for _ in range(30)]' %clang_cc1 -std=c++29 -I %S/../Reflection/Inputs -fsyntax-only -code-completion-at=%s:25:27 %s -o -

#include "token-sequence.h"
using std::meta::token_sequence;

__macro use(token_sequence expr) { return ^^{ \(expr) }; }
__macro ignore(token_sequence expr) { return ^^{ 0 }; }
__macro statement(token_sequence expr) { return ^^{ int value = \(expr); }; }
__macro declaration(token_sequence type) { return ^^{ \(type) value; }; }
int partial_value;
struct ExistingType {};
int run() { return use!(1 + partial_value); }
// EXPR: COMPLETION: partial_value : [#int#]partial_value
int ignored() { return ignore!(partial_value); }
// DROP-NOT: COMPLETION:
void run_statement() { statement!(partial_value); }
// STMT: COMPLETION: partial_value : [#int#]partial_value
declaration!(ExistingType);
// DECL: COMPLETION: ExistingType : ExistingType
int first() { return use!( + 1); }
// FIRST: COMPLETION: partial_value : [#int#]partial_value
